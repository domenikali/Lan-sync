FROM gcc:12.2.0 AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends cmake libssl-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY server/ ./server/

RUN cmake -S ./server -B ./server/build && \
    cmake --build ./server/build -j$(nproc)

FROM gcc:12.2.0 AS runtime

RUN apt-get purge -y --auto-remove gcc g++ && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y libssl1.1 libsqlite3-0 && rm -rf /var/lib/apt/lists/* 

WORKDIR /app
COPY --from=builder /app/server/build/lan_sync_server .
COPY server/web ./web

EXPOSE 8080

ENV SSD_CACHE_PATH=/data/ssd_cache \
    HDD_STORAGE_PATH=/data/hdd_storage

CMD ["./lan_sync_server"]
