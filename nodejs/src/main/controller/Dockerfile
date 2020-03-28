# 5
FROM arm32v7/ubuntu:latest

ENV TZ America/New_York

WORKDIR /usr/src/app

RUN apt-get update
RUN apt-get install -y python make g++

COPY {{NODEJS_DOWNLOAD_FILENAME}} ./{{NODEJS_DOWNLOAD_FILENAME}}
RUN tar xvf {{NODEJS_DOWNLOAD_FILENAME}}
RUN rm -f {{NODEJS_DOWNLOAD_FILENAME}}

ENV PATH /usr/src/app/{{NODEJS_DOWNLOAD_DIRECTORY}}/bin/:$PATH

COPY package*.json ./
COPY index.js ./

RUN npm --unsafe-perm install

CMD ["npm", "start"]
