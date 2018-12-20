FROM ubuntu:18.04
LABEL Name=patriots Version=0.0.1

RUN apt-get update
RUN apt-get -y install build-essential
RUN apt-get -y install liballegro4.4 liballegro4-dev

WORKDIR /app
COPY . .
RUN mkdir build
RUN make

CMD [ "./build/patriots" ]