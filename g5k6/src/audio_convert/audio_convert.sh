#/bin/sh
./audio_convert -i speaker.pcm      -o speaker.g711A        -f 0 -k 1 -r 8000
./audio_convert -i speaker.pcm      -o speaker.g711U        -f 0 -k 2 -r 8000
./audio_convert -i speaker.pcm      -o speaker.g726-16K     -f 0 -k 3 -r 8000
./audio_convert -i speaker.pcm      -o speaker.g726-32K     -f 0 -k 4 -r 8000
./audio_convert -i speaker.pcm      -o speaker.aac          -f 0 -k 5 -r 8000

./audio_convert -i speaker.g711A    -o speaker.g711A.pcm    -f 1 -k 0 -r 8000
./audio_convert -i speaker.g711U    -o speaker.g711U.pcm    -f 2 -k 0 -r 8000
./audio_convert -i speaker.g726-16K -o speaker.g726-16K.pcm -f 3 -k 0 -r 8000
./audio_convert -i speaker.g726-32K -o speaker.g726-32K.pcm -f 4 -k 0 -r 8000
./audio_convert -i speaker.aac      -o speaker.aac.pcm      -f 5 -k 0 -r 8000
