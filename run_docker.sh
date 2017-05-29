set -e
docker build -t distman ./docker/
docker run -i -t --rm --privileged -v $PWD:/tmp/m -w /tmp/m distman
