all:
	if [ `getconf LONG_BIT` -eq 32 ]; then make -C libturbojpeg-32; else make -C libturbojpeg-64; fi
clean:
	if [ `getconf LONG_BIT` -eq 32 ]; then make -C libturbojpeg-32 clean; else make -C libturbojpeg-64 clean; fi

