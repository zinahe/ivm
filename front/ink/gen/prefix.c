#include "pub/const.h"

#include "vm/native/native.h"

#include "util/opt.h"

#include "priv.h"

ilang_gen_value_t
ilang_gen_fn_expr_eval(ilang_gen_expr_t *expr,
					   ilang_gen_flag_t flag,
					   ilang_gen_env_t *env)
{
	ilang_gen_fn_expr_t *func = IVM_AS(expr, ilang_gen_fn_expr_t);
	ivm_exec_t *exec, *exec_backup;
	ivm_size_t exec_id, olen;
	ilang_gen_param_t *tmp_param1, *tmp_param2;
	ilang_gen_param_list_t *params;
	ilang_gen_param_list_iterator_t piter, chk_iter;
	ivm_char_t *tmp_str;
	ivm_bool_t has_varg = IVM_FALSE;
	ivm_int_t cur_param, param_count;
	ivm_size_t sp_back;
	const ivm_char_t *err;
	ivm_size_t tmp_addr;

	ilang_gen_addr_set_t addr_backup = env->addr;
	sp_back = env->sp;
	env->sp = 0;

	GEN_ASSERT_NOT_LEFT_VALUE(expr, "function expression", flag);

	params = func->params;

	if (params) {
		param_count = ilang_gen_param_list_size(params);
	} else {
		param_count = 0;
	}

	exec = ivm_exec_new(env->str_pool, param_count);
	exec_id = ivm_exec_unit_registerExec(env->unit, exec);
	exec_backup = env->cur_exec;
	env->cur_exec = exec;

	env->addr = ilang_gen_addr_set_init();

	// ivm_exec_setSourcePos(exec, env->file);

	/*
		ink calling convention:
			top                                           bottom
			-------------------     ----------------------------
			| arg n | arg n-1 | ... | arg 1 | func | base(opt) |
			-------------------     ----------------------------
	 */

	if (params) {
		cur_param = 0;

		ILANG_GEN_PARAM_LIST_EACHPTR_R(params, piter) {
			tmp_param1 = ILANG_GEN_PARAM_LIST_ITER_GET_PTR(piter);

			ILANG_GEN_PARAM_LIST_EACHPTR_R(params, chk_iter) {
				tmp_param2 = ILANG_GEN_PARAM_LIST_ITER_GET_PTR(chk_iter);

				if (tmp_param1 != tmp_param2 &&
					!IVM_STRNCMP(tmp_param1->name.val, tmp_param1->name.len,
								 tmp_param2->name.val, tmp_param2->name.len)) {
					GEN_ERR_DUP_PARAM_NAME(expr, tmp_param2->name.val, tmp_param2->name.len);
				}
			}

			tmp_str = ivm_parser_parseStr_heap_n(env->heap, tmp_param1->name.val,
												 tmp_param1->name.len, &err, &olen);
			
			if (!tmp_str) {
				GEN_ERR_FAILED_PARSE_STRING(expr, err);
			}

			if (tmp_param1->is_varg && has_varg) {
				GEN_ERR_MULTIPLE_VARG(expr);
			}

			ivm_exec_setParam(
				exec, cur_param,
				ivm_exec_registerString_nc(exec, tmp_str ? tmp_str : IVM_NATIVE_VARG_NAME, olen),
				tmp_param1->is_varg
			);

			if (tmp_param1->def) {
				ivm_exec_addInstr_nl(exec, GET_LINE(expr), GET_LOCAL_SLOT, olen, tmp_str);
				tmp_addr = ivm_exec_addInstr_l(exec, GET_LINE(expr), CHECK_NONE, 0);
				
				tmp_param1->def->eval(tmp_param1->def, FLAG(0), env);
				ivm_exec_addInstr_nl(exec, GET_LINE(expr), SET_LOCAL_SLOT, olen, tmp_str);
				
				ivm_exec_setArgAt(exec, tmp_addr, ivm_exec_cur(exec) - tmp_addr);
			}

			cur_param++;
		}
	}

	func->body->eval(
		func->body,
		FLAG(0), // function body is not top-level(need last value)
		env
	);

	env->cur_exec = exec_backup;
	ivm_opt_optExec(exec);

	env->addr = addr_backup;
	env->sp = sp_back;

	if (!flag.is_top_level) {
		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), NEW_FUNC, exec_id);
	}

	return NORET();
}

ilang_gen_value_t
ilang_gen_varg_expr_eval(ilang_gen_expr_t *expr,
						 ilang_gen_flag_t flag,
						 ilang_gen_env_t *env)
{
	ilang_gen_varg_expr_t *varg = IVM_AS(expr, ilang_gen_varg_expr_t);

	// GEN_ASSERT_ONLY_LEFT_VAL(expr, flag, "varg expression");
	// GEN_ASSERT_ONLY_LIST(expr, flag, "varg expression");
	GEN_ASSERT_VARG_ENABLE(expr, flag)

	if (flag.is_left_val) {
		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), NEW_LIST_ALL_R, flag.varg_offset - 1);
		varg->bondee->eval(varg->bondee, FLAG(.is_left_val = IVM_TRUE), env);
	} else {
		varg->bondee->eval(varg->bondee, FLAG(0), env);
		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), TO_LIST);

		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), UNPACK_LIST_ALL);
	}

	return NORET();
}

ilang_gen_value_t
ilang_gen_intr_expr_eval(ilang_gen_expr_t *expr,
						 ilang_gen_flag_t flag,
						 ilang_gen_env_t *env)
{
	ilang_gen_intr_expr_t *intr = IVM_AS(expr, ilang_gen_intr_expr_t);

	GEN_ASSERT_NOT_LEFT_VALUE(expr, "interrupt expression", flag);
	// GEN_ASSERT_NO_NESTED_RET(expr, flag)

	ivm_size_t cur = ivm_exec_cur(env->cur_exec);

	switch (intr->sig) {
		case ILANG_GEN_INTR_RET:
			if (intr->val) {
				intr->val->eval(intr->val, FLAG(0), env);
			} else {
				ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), NEW_NONE);
			}
			ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), RETURN);

			break;

		case ILANG_GEN_INTR_CONT:
			if (env->addr.continue_addr != -1) {
				if (intr->val) {
					GEN_WARN_GENERAL(expr, GEN_ERR_MSG_BREAK_OR_CONT_IGNORE_ARG);
				}

				ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), INT_LOOP, env->addr.nl_block);
				cur = ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), JUMP, 0);

				ivm_exec_setArgAt(env->cur_exec, cur, env->addr.continue_addr - cur);

				/*
				if (env->addr.nl_block) {
					ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), INT_N_LOOP, env->addr.nl_block);
					ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), JUMP, env->addr.continue_addr - cur);
				} else {
					ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), INT_LOOP, env->addr.continue_addr - cur);
				}
				*/
			} else {
				GEN_ERR_GENERAL(expr, GEN_ERR_MSG_BREAK_OR_CONT_OUTSIDE_LOOP);
			}

			break;

		case ILANG_GEN_INTR_BREAK:
			if (env->addr.break_ref) {
				if (intr->val) {
					GEN_WARN_GENERAL(expr, GEN_ERR_MSG_BREAK_OR_CONT_IGNORE_ARG);
				}

				ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), INT_LOOP, env->addr.nl_block);
				cur = ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), JUMP, 0);

				/*
				if (env->addr.nl_block) {
					ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), INT_N_LOOP, env->addr.nl_block);
					cur = ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), JUMP, 0);
				} else {
					ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), INT_LOOP, 0);
				}
				*/
			
				ilang_gen_addr_list_push(env->addr.break_ref, cur);
			} else {
				GEN_ERR_GENERAL(expr, GEN_ERR_MSG_BREAK_OR_CONT_OUTSIDE_LOOP);
			}

			break;
	
		case ILANG_GEN_INTR_RAISE:
			intr->val->eval(intr->val, FLAG(0), env);
			ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), RAISE);

			break;

		case ILANG_GEN_INTR_RESUME:
			intr->val->eval(intr->val, FLAG(0), env);
			
			if (intr->with) {
				intr->with->eval(intr->with, FLAG(0), env);
			} else {
				ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), NEW_NONE);
			}

			ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), RESUME);

			if (flag.is_top_level) {
				ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), POP);
			}

			break;

		case ILANG_GEN_INTR_YIELD:
			if (intr->val) {
				intr->val->eval(intr->val, FLAG(0), env);
			} else {
				ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), NEW_NONE);
			}
			
			ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), YIELD);
			
			if (flag.is_top_level) {
				ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), POP);
			}

			break;

		default:
			IVM_FATAL("unsupported interrupt signal");
	}

	return NORET();
}

ilang_gen_value_t
ilang_gen_assert_expr_eval(ilang_gen_expr_t *expr,
						   ilang_gen_flag_t flag,
						   ilang_gen_env_t *env)
{
	ilang_gen_assert_expr_t *asrt = IVM_AS(expr, ilang_gen_assert_expr_t);

	GEN_ASSERT_NOT_LEFT_VALUE(expr, "assert expression", flag);

	asrt->cond->eval(asrt->cond, FLAG(0), env);

	if (flag.is_top_level) {
		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), ASSERT_TRUE);
	} else {
		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), ASSERT_TRUE_N); // no pop
	}

	return NORET();
}

ilang_gen_value_t
ilang_gen_assign_expr_eval(ilang_gen_expr_t *expr,
						   ilang_gen_flag_t flag,
						   ilang_gen_env_t *env)
{
	ilang_gen_assign_expr_t *assign = IVM_AS(expr, ilang_gen_assign_expr_t);

	// assign expression itself should not be left value
	GEN_ASSERT_NOT_LEFT_VALUE(expr, "assign expression", flag);

	if (assign->inp_op != -1) {
		assign->lhe->eval(assign->lhe, FLAG(0), env);
		assign->rhe->eval(assign->rhe, FLAG(0), env);

#define INPLACE_OP(op) \
	case IVM_BINOP_ID(op): \
		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), op); \
		break;

		switch (assign->inp_op) {
			INPLACE_OP(INADD)
			INPLACE_OP(INSUB)
			INPLACE_OP(INMUL)
			INPLACE_OP(INDIV)
			INPLACE_OP(INMOD)

			INPLACE_OP(INAND)
			INPLACE_OP(INIOR)
			INPLACE_OP(INEOR)

			INPLACE_OP(INSHL)
			INPLACE_OP(INSHAR)
			INPLACE_OP(INSHLR)

			default:
				IVM_FATAL("impossible");
		}
#undef INPLACE_OP

		if (!flag.is_top_level) {
			ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), DUP);
		}

		assign->lhe->eval(assign->lhe, FLAG(.is_left_val = IVM_TRUE), env);
	} else {
		assign->rhe->eval(assign->rhe, FLAG(0), env);
		INC_SP();
		if (!flag.is_top_level) {
			INC_SP();
			ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), DUP);
		}
		// ilang_gen_leftval_eval(assign->lhe, expr, env);
		assign->lhe->eval(assign->lhe, FLAG(.is_left_val = IVM_TRUE), env);

		DEC_SP();
		if (!flag.is_top_level) {
			DEC_SP();
		}
	}

	return NORET();
}

ilang_gen_value_t
ilang_gen_fork_expr_eval(ilang_gen_expr_t *expr,
						 ilang_gen_flag_t flag,
						 ilang_gen_env_t *env)
{
	ilang_gen_fork_expr_t *fork_expr = IVM_AS(expr, ilang_gen_fork_expr_t);
	
	GEN_ASSERT_NOT_LEFT_VALUE(expr, "fork expression", flag);

	fork_expr->forkee->eval(fork_expr->forkee, FLAG(0), env);
	ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), FORK);

	if (flag.is_top_level) {
		ivm_exec_addInstr_l(env->cur_exec, GET_LINE(expr), POP);
	}

	return NORET();
}
