set -e
docker build -t distman ./docker/
docker run -p2181:2181 -i -t --rm --privileged -v $PWD:/tmp/m -w /tmp/m distman
