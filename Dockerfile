ARG RESTY_TAG="1.25.3.1-alpine"
FROM openresty/openresty:${RESTY_TAG} AS builder


RUN apk add --no-cache musl-dev gcc make git openssl-dev libuuid util-linux-dev redis

ADD --chown=redis src /app

RUN cd /app && make


FROM openresty/openresty:${RESTY_TAG}

RUN sed -i 's/worker_connections\s*1024/worker_connections 10240/' /usr/local/openresty/nginx/conf/nginx.conf

WORKDIR /app
VOLUME /app

COPY public/pow.min.js public/index.html /var/www/html/
COPY public/sdk.min.js /var/www/html/sdk.js
COPY src/challenge.conf /etc/nginx/conf.d/default.conf
COPY --from=builder /app/powmodule.so /var/lib/redis/powmodule.so
COPY --from=builder /usr/bin/redis-server /usr/bin/redis-server

ENTRYPOINT ["/bin/sh", "-c" , "nginx && redis-server --dir /app --loadmodule /var/lib/redis/powmodule.so --port 6379 --daemonize no --bind 0.0.0.0 --protected-mode no"]

