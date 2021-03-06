import std

a = 1
b = 2

del a

print(b) // -> "num: 2"

a = "no!"
f = fn: {
	loc a = "wow"
	del a
	a = "yes!"
}
i = 0

while i < 100000: {
	f()
	i = i + 1
}

print(a) // -> "str: yes!"

a = "outermost"

f1 = fn: {
	loc a = "out"
	f2 = fn: {
		a = "inside"
	}
	f2()
	print(a) // -> "str: inside"
	del a
	print(a) // -> "str: outermost"
}

f1()

a = {
	val: 0
}

del a["val"]
print(typename(a.val))

a = 1
b = 2
c = 10

print(b)

del [ a, [ b, c, a ], c ]

print(b)

try:
	del [ *a ]
catch: print("catched")

// -> "str: none"

// -> "num: 2"
// -> "<none>"
// -> "str: catched"
