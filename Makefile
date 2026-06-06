b:
	pebble clean && pebble build

p:
	pebble install --phone 192.168.178.117

e:
	pebble install --emulator emery

e1:
	pebble install --emulator flint

bp: b p

be: b e

be1: b e1
