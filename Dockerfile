# ---------- Stage 1: compile the C++ program ----------
FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends g++ && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY server.cpp .
RUN g++ -std=c++11 -O2 -static-libstdc++ -static-libgcc server.cpp -o library

# ---------- Stage 2: small runtime image ----------
FROM debian:bookworm-slim
ARG APP_VERSION=dev
ENV APP_VERSION=${APP_VERSION} PORT=8080 BIND_ADDR=0.0.0.0
RUN useradd --create-home --uid 10001 appuser
WORKDIR /app
COPY --from=build /src/library .
COPY books.csv .
COPY web ./web
RUN chown -R appuser:appuser /app
USER appuser
EXPOSE 8080
CMD ["./library"]
