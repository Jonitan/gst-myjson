first install plugin myjson:
1. switch to projects main working space directory.
2. run: meson build
3. run: ninja -C build install

now plugin should be installed, you can check with:
1. run: gst-inspect-1.0 build/gst-plugin/libgstmyjson.so

now set-up plugin env, compile and run test:
1. run: export GST_PLUGIN_PATH="/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0/"
2. run: /usr/bin/gcc tests/test.c -g -o test `pkg-config --cflags --libs gstreamer-1.0 gstreamer-audio-1.0`
3. run: ./test


Notes:
when mp3/amr/flac used before json added, even with num-buffers=1 few buffers are sent, therefor json added for each time.