mkdir out_arm_ios
mkdir out_arm_ios64
mkdir out_intel_ios64

rm -rf out

mv out_arm_ios out
./configure --prefix=/jxcoreIOSarm --static-library --dest-os=ios --dest-cpu=arm --engine-mozilla

rm -rf /jxcoreIOSarm/bin
make install

mv /jxcoreIOSarm/bin/libcares.a /jxcoreIOSarm/bin/libcares_device.a
mv /jxcoreIOSarm/bin/libchrome_zlib.a /jxcoreIOSarm/bin/libchrome_zlib_device.a
mv /jxcoreIOSarm/bin/libhttp_parser.a /jxcoreIOSarm/bin/libhttp_parser_device.a
mv /jxcoreIOSarm/bin/libjx.a /jxcoreIOSarm/bin/libjx_device.a
mv /jxcoreIOSarm/bin/libmozjs.a /jxcoreIOSarm/bin/libmozjs_device.a
mv /jxcoreIOSarm/bin/libopenssl.a /jxcoreIOSarm/bin/libopenssl_device.a
mv /jxcoreIOSarm/bin/libuv.a /jxcoreIOSarm/bin/libuv_device.a
mv /jxcoreIOSarm/bin/libsqlite3.a /jxcoreIOSarm/bin/libsqlite3_device.a

mv out out_arm_ios

mv out_arm_ios64 out
./configure --prefix=/jxcoreIOSarm64 --static-library --dest-os=ios --dest-cpu=arm64 --engine-mozilla

rm -rf /jxcoreIOSarm64/bin
make install
mv /jxcoreIOSarm64/bin/libcares.a /jxcoreIOSarm64/bin/libcares_device64.a
mv /jxcoreIOSarm64/bin/libchrome_zlib.a /jxcoreIOSarm64/bin/libchrome_zlib_device64.a
mv /jxcoreIOSarm64/bin/libhttp_parser.a /jxcoreIOSarm64/bin/libhttp_parser_device64.a
mv /jxcoreIOSarm64/bin/libjx.a /jxcoreIOSarm64/bin/libjx_device64.a
mv /jxcoreIOSarm64/bin/libmozjs.a /jxcoreIOSarm64/bin/libmozjs_device64.a
mv /jxcoreIOSarm64/bin/libopenssl.a /jxcoreIOSarm64/bin/libopenssl_device64.a
mv /jxcoreIOSarm64/bin/libuv.a /jxcoreIOSarm64/bin/libuv_device64.a
mv /jxcoreIOSarm64/bin/libsqlite3.a /jxcoreIOSarm64/bin/libsqlite3_device64.a

mv out out_arm_ios64

mv out_intel_ios64 out
./configure --prefix=/jxcoreIOSintel64 --static-library --dest-os=ios --dest-cpu=x64 --engine-mozilla

rm -rf /jxcoreIOSintel64/bin
make install

mv /jxcoreIOSintel64/bin/libcares.a /jxcoreIOSintel64/bin/libcares_simulator64.a
mv /jxcoreIOSintel64/bin/libchrome_zlib.a /jxcoreIOSintel64/bin/libchrome_zlib_simulator64.a
mv /jxcoreIOSintel64/bin/libhttp_parser.a /jxcoreIOSintel64/bin/libhttp_parser_simulator64.a
mv /jxcoreIOSintel64/bin/libjx.a /jxcoreIOSintel64/bin/libjx_simulator64.a
mv /jxcoreIOSintel64/bin/libmozjs.a /jxcoreIOSintel64/bin/libmozjs_simulator64.a
mv /jxcoreIOSintel64/bin/libopenssl.a /jxcoreIOSintel64/bin/libopenssl_simulator64.a
mv /jxcoreIOSintel64/bin/libuv.a /jxcoreIOSintel64/bin/libuv_simulator64.a
mv /jxcoreIOSintel64/bin/libsqlite3.a /jxcoreIOSintel64/bin/libsqlite3_simulator64.a
 
rm -rf /jxcoreIOS
mkdir -p /jxcoreIOS/bin
mv /jxcoreIOSarm/include /jxcoreIOS
cp deps/mozjs/src/js.msg /jxcoreIOS/include/node/
lipo -create "/jxcoreIOSarm64/bin/libcares_device64.a" "/jxcoreIOSarm/bin/libcares_device.a" "/jxcoreIOSintel64/bin/libcares_simulator64.a" -output "/jxcoreIOS/bin/libcares.a"
lipo -create "/jxcoreIOSarm64/bin/libchrome_zlib_device64.a" "/jxcoreIOSarm/bin/libchrome_zlib_device.a" "/jxcoreIOSintel64/bin/libchrome_zlib_simulator64.a" -output "/jxcoreIOS/bin/libchrome_zlib.a"
lipo -create "/jxcoreIOSarm64/bin/libhttp_parser_device64.a" "/jxcoreIOSarm/bin/libhttp_parser_device.a" "/jxcoreIOSintel64/bin/libhttp_parser_simulator64.a" -output "/jxcoreIOS/bin/libhttp_parser.a"
lipo -create "/jxcoreIOSarm64/bin/libjx_device64.a" "/jxcoreIOSarm/bin/libjx_device.a" "/jxcoreIOSintel64/bin/libjx_simulator64.a" -output "/jxcoreIOS/bin/libjx.a"
lipo -create "/jxcoreIOSarm64/bin/libmozjs_device64.a" "/jxcoreIOSarm/bin/libmozjs_device.a" "/jxcoreIOSintel64/bin/libmozjs_simulator64.a" -output "/jxcoreIOS/bin/libmozjs.a"
lipo -create "/jxcoreIOSarm64/bin/libopenssl_device64.a" "/jxcoreIOSarm/bin/libopenssl_device.a" "/jxcoreIOSintel64/bin/libopenssl_simulator64.a" -output "/jxcoreIOS/bin/libopenssl.a"
lipo -create "/jxcoreIOSarm64/bin/libuv_device64.a" "/jxcoreIOSarm/bin/libuv_device.a" "/jxcoreIOSintel64/bin/libuv_simulator64.a" -output "/jxcoreIOS/bin/libuv.a"
lipo -create "/jxcoreIOSarm64/bin/libsqlite3_device64.a" "/jxcoreIOSintel64/bin/libsqlite3_simulator64.a" -output "/jxcoreIOS/bin/libsqlite3.a"
 
mv out out_intel_ios64


