#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

using json = nlohmann::json;

long MAX_SCAN_COUNT = 1000000; 

class DocumetDb {
private:
    redisContext* ctx;
    std::string m_name;
public:
    explicit DocumetDb(std::string str){
      connect();
    }
    ~DocumetDb() {}
    
    redisContext* get_context(){
      return ctx;
    }

    int connect(){
      ctx = redisConnect("127.0.0.1", 6379);
      if (ctx == nullptr || ctx->err) {
          std::cerr << "Connection error: "
                    << (ctx ? ctx->errstr : "null context") << std::endl;
          if (ctx) redisFree(ctx);
          return 1;
      }      
      return -1;
    }

    void free_ctx(){
        if (ctx) redisFree(ctx);
    }

    std::vector<EmbedData> get_scan_items(std::string prefix, long max_count)
    {
      std::vector<EmbedData> ret;
      // SCAN開始
      std::string cursor = "0";
      std::vector<EmbedData> scanItems;
      std::string match_str = prefix + "*";

      do
      {
          EmbedData row;
          //"SCAN %llu MATCH %s COUNT %d", %ld
          redisReply* reply = static_cast<redisReply*>(
              redisCommand(
                  ctx,
                  "SCAN %s MATCH %s COUNT %ld",
                  cursor.c_str(),
                  match_str.c_str() ,
                  max_count
              )
          );

          if (reply == nullptr)
          {
              std::cerr << "SCAN command failed" << std::endl;
              break;
          }

          if (reply->type != REDIS_REPLY_ARRAY ||
              reply->elements != 2)
          {
              std::cerr << "Invalid SCAN response" << std::endl;
              freeReplyObject(reply);
              break;
          }

          // 次のcursor
          redisReply* cursorReply = reply->element[0];

          cursor = cursorReply->str;

          // KEY一覧
          redisReply* keysReply = reply->element[1];

          for (size_t i = 0; i < keysReply->elements; i++)
          {
              redisReply* keyReply = keysReply->element[i];

              std::string key = keyReply->str;

              // VALUE取得
              redisReply* valueReply = static_cast<redisReply*>(
                  redisCommand(
                      ctx,
                      "GET %s",
                      key.c_str()
                  )
              );

              if (valueReply == nullptr)
              {
                  std::cerr << "GET failed: " << key << std::endl;
                  continue;
              }

              std::cout << "KEY   : " << key << std::endl;

              if (valueReply->type == REDIS_REPLY_STRING)
              {
                  std::cout << "VALUE : "
                            << valueReply->str
                            << std::endl;
                  std::string j1_str = valueReply->str;
                  json j1 = json::parse(j1_str);
                  std::string content = j1.at("content").get<std::string>();                  
                  std::string id = j1.at("id").get<std::string>();                  
                  std::string vector = j1.at("vector").get<std::string>();                  
                  json j2 = json::parse(vector);
                  auto vec = j2;
                  //int vlength = sizeof(vec) / sizeof(vec[0]);
                  row.embedding    = vec.get<std::vector<float>>();                  
                  row.id = id;
                  row.content = content;
                  scanItems.push_back(row);
              }
              else if (valueReply->type == REDIS_REPLY_NIL)
              {
                  std::cout << "VALUE : (nil)" << std::endl;
              }
              else
              {
                  std::cout << "VALUE : unsupported type"
                            << std::endl;
              }

              std::cout << "----------------------"
                        << std::endl;

              freeReplyObject(valueReply);
          }

          freeReplyObject(reply);

      } while (cursor != "0");

      std::cout << "scanItems.size=" << scanItems.size() << std::endl;

      ret = scanItems;
      return ret;
    }

    float cosine_similarity(const std::vector<float>& v1, const std::vector<float>& v2) {
        if (v1.size() != v2.size()) {
            throw std::invalid_argument("Vectors must be of the same length.");
        }

        float dot = 0.0f, norm1Sq = 0.0f, norm2Sq = 0.0f;

        for (size_t i = 0; i < v1.size(); i++) {
            const float a = v1[i];
            const float b = v2[i];
            dot     += a * b;
            norm1Sq += a * a;
            norm2Sq += b * b;
        }

        return dot / (std::sqrt(norm1Sq) * std::sqrt(norm2Sq));
    }    

    bool get_one_list(std::string prefix, std::string vec_str) {
        bool ret = false;
        try {
            json j1 = json::parse(vec_str);
            std::cout << "size: " << j1.size() << '\n';
            auto embedding = j1;
            int vlen = sizeof(embedding) / sizeof(embedding[0]);
            std::cout << "embedding.vlen=" << embedding.size() << std::endl;            
            auto items = get_scan_items(prefix, 5);
            std::cout << "items.size=" << items.size() << std::endl;
            if(items.size() == 0){
                return true;
            }

            std::vector<ResultEmbed> result_items;
            int one_vec_size = 0;
            for (const auto& data : items) {
                std::string id = data.id;
                std::vector<float> vec = data.embedding;

                ResultEmbed res_item;
                res_item.id = id;
                res_item.embedding = vec;
                res_item.content = data.content;
                if(result_items.size() == 0){
                    one_vec_size = vec.size();
                    std::cout << "one_vec_size=" << one_vec_size << std::endl;
                    result_items.push_back(res_item); 
                }
            }
            if(embedding.size() != one_vec_size){
                std::cout << "error, embedding.size NG" << std::endl;
                return ret;
            }
            return true;
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return ret;
        }
        return ret;
    }

    std::string getTableList(std::string  prefix ,std::string vec_str, int limit) {
        std::string ret = "";
        try {
            json j1 = json::parse(vec_str);
            std::cout << "size: " << j1.size() << '\n';
            auto embedding = j1;
            int vlen = sizeof(embedding) / sizeof(embedding[0]);
            std::cout << "embedding.vlen=" << embedding.size() << std::endl;            
            auto items = get_scan_items(prefix, MAX_SCAN_COUNT);
            std::cout << "items.size=" << items.size() << std::endl;

            std::vector<ResultEmbed> result_items;
            for (const auto& data : items) {
                std::string id = data.id;
                std::vector<float> vec = data.embedding;
                int vlength = sizeof(vec) / sizeof(vec[0]);

                float distance = cosine_similarity(embedding, vec);
                std::cout << "distance=" << distance << std::endl;            
                ResultEmbed res_item;
                res_item.id = id;
                res_item.embedding = vec;
                res_item.content = data.content;
                res_item.distance = distance;
                if(distance > 0.4) {
                    result_items.push_back(res_item);
                }
            }
            std::cout << "result_items.vlen=" << result_items.size() << std::endl;

            //sort
            std::sort(result_items.begin(), result_items.end(),
                [](const ResultEmbed& a, const ResultEmbed& b) {
                    return a.distance > b.distance;
                }
            );   
            std::vector<SearchDbItem> out_items;
            for (const auto& item : result_items) {
                if (out_items.size() < limit) {
                    std::cout << "distance=" << item.distance
                        << ", id=" << item.id << std::endl;
                    SearchDbItem db_row;
                    db_row.id = item.id;
                    db_row.distance = item.distance;
                    db_row.content = item.content;
                    out_items.push_back(db_row);
                }        
            }
            json j2 = out_items;
            std::string json_str = j2.dump();
            std::cout << json_str << std::endl;            
            ret = json_str;
            return ret;
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return ret;
        }
        return ret;
    }     

    std::string get_uuid(){
        uuid_t uuid;
        char uuid_str[37]; // 36文字 + NULL

        // UUID生成（ランダム）
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);

        std::cout << "UUID: " << uuid_str << std::endl;         
        std::string new_id= uuid_str;
        return new_id;
    }   

    void vector_delete(
      const std::string& prefix , const std::string& id
    ){
      try{  
        std::string key_str = prefix + id;
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(
                ctx,
                "DEL %s",
                key_str.c_str()
            )
        );
        if (reply == nullptr) {
            std::cerr << "SET failed" << std::endl;
            redisFree(ctx);
            return;
        }
        if (reply->type == REDIS_REPLY_STATUS) {
            std::cout << "SET result: "
                      << reply->str << std::endl;
        }
        freeReplyObject(reply);
        redisFree(ctx);        

      } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
      }
    }

    void vector_add(
      const std::string& prefix , const std::string& content, std::string embedding
    ){
      try{  

        std::string new_id= get_uuid();
        std::cout << "UUID: " << new_id << std::endl;  
        json data = {
            {"id", new_id.c_str() },
            {"content", content.c_str() },
            {"vector", embedding.c_str() }
        };

        std::string jsonText = data.dump();
        std::string key_str = prefix + new_id;
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(
                ctx,
                "SET %s %s",
                key_str.c_str(),
                jsonText.c_str()
            )
        );
        if (reply == nullptr) {
            std::cerr << "SET failed" << std::endl;
            redisFree(ctx);
            return;
        }
        if (reply->type == REDIS_REPLY_STATUS) {
            std::cout << "SET result: "
                      << reply->str << std::endl;
        }
        freeReplyObject(reply);
        redisFree(ctx);        

      } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
      }
    }

};
