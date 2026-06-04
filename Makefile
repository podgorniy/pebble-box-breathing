b:
	pebble clean && pebble build

p:
	pebble install --phone 192.168.178.117

e:
	pebble install --emulator emery

bp: b p

be: b e
