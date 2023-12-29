all: tree

tree:
	cd BACnet ; tree -T "BACnet Files" -H https://kargs.net/BACnet  > index.html ; cd ..
	cd captures ; tree -T "Wireshark Capture Files" -H https://kargs.net/captures  > index.html ; cd ..
	cd code ; tree -T "Source Code Files" -H https://kargs.net/code  > index.html ; cd ..
	cd docs ; tree -T "Document Files" -H https://kargs.net/docs  > index.html ; cd ..

zip:
	# note: cleanup zip files
	cd zip ; tree -T "Compressed Files" -H https://kargs.net/zip  > index.html ; cd ..
