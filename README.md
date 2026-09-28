# cpp_redis_api_vec1

 Version: 0.9.1

 date    : 2026/09/27
 
 update :

***

C++ Redis Api Server , Vector add search

* Redis Database
* LLVM CLang
* cpp-httplib
* make
* Linux

***
### related Client

* http client , RAG app

https://github.com/kuc-arc-f/cpp_16ex/tree/main/redis_cl_2

***
### related

https://github.com/yhirose/cpp-httplib

***
* LIB add
```
sudo apt update
sudo apt-get install libhiredis-dev
sudo apt-get install nlohmann-json3-dev
sudo apt install libspdlog-dev libfmt-dev
```

***
* build
```
make all
```

* start , localhost:8888

```
./api_redis_server
```

***
### Test- code
* vector add
* content: text
* vector: vector data

```
curl -X POST http://localhost:8888/api/insert \
  -H "Content-Type: application/json" \
  -d '{"content": "hello", "vector": "[0.01 , 0.03, 0.04]"}'
```

***
* vector select
* limit: max record
* vector: vector data
```
curl -X POST http://localhost:8888/api/select \
  -H "Content-Type: application/json" \
  -d '{"limit": 3 ,"vector": "[0.02 , 0.03, 0.14]"}'
```

* vector delete
* id: id value

```
curl -X POST http://localhost:8888/api/delete \
  -H "Content-Type: application/json" \
  -d '{"id": "99d473b7-4f06-4a6d-b9e6-e35821e71df2"}'

```

***
### blog

