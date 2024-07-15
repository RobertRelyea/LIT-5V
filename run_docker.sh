docker run -d -it --name pico_env --mount type=bind,source=${PWD},target=/home/dev --network host pico_env:0.1
docker exec -it pico_env /bin/bash

docker kill pico_env
docker rm pico_env
