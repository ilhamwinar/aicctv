#FROM yudhabhakti/avc-ai

FROM pre-aicctv

#RUN apt-get -y update && apt-get install -y

#RUN apt-get install libeigen3-dev libssl-dev -y

#RUN wget https://cmake.org/files/v3.18/cmake-3.18.6.tar.gz

#RUN tar -zxvf cmake-3.18.6.tar.gz

#WORKDIR /workspace/cmake-3.18.6/

#RUN ./configure

#RUN make

#RUN make install

#WORKDIR /

#Install AMQP
#RUN wget https://github.com/alanxz/rabbitmq-c/archive/refs/tags/v0.11.0.tar.gz

#RUN tar -zxvf v0.11.0.tar.gz

#WORKDIR /rabbitmq-c-0.11.0/build



#RUN cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..

#RUN cmake --build . --target install

#RUN ldconfig

#WORKDIR /

#RUN apt-get install git -y

#RUN git clone https://github.com/bhaktiyudha/yolov5-deepsort-tensorrt.git

#RUN ls

#RUN mkdir /yolov5-deepsort-tensorrt/build

#COPY . /yolov5-deepsort-tensorrt/

#WORKDIR /yolov5-deepsort-tensorrt/build

#RUN ls

#RUN cmake ..

#RUN cmake --build .

#RUN ls

#EXPOSE 85

#CMD ["./yolosort"]

CMD ["/bin/sh"]
