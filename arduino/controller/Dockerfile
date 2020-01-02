FROM docker.pkg.github.com/wgillaspy/arm64v8-avrdude/arm64v8-avrdude:latest

ENV TZ America/New_York

WORKDIR /usr/app

COPY controller.ino /usr/app/controller.ino

CMD ["bash", "/arduino-builder.sh","controller.ino", "/dev/ttyUSB0"]
