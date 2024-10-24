set -e

export DISPLAY=:0
cd /project/gnubg && \
make -j5 && \
./gnubg