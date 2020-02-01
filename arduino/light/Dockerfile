FROM docker.pkg.github.com/wgillaspy/arm64v8-avrdude/arm64v8-avrdude:latest

ENV TZ America/New_York

WORKDIR /usr/app

COPY light.ino /usr/app/light.ino

CMD ["bash", "/arduino-builder.sh","light.ino", "/dev/lightController"]
