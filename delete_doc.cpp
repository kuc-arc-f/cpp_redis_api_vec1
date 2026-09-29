#include <iostream>
#include <string>
#include <hiredis/hiredis.h>

int main(int argc, char* argv[])
{
    if(argc < 2) {
        std::cerr << "[ERROR] argment none" << "\n";
        return -1;
    }
    std::string prefix = argv[1];
    std::cout << "prefix=" << prefix << std::endl;
    //return -1;    
    // Redisへ接続
    redisContext* redis = redisConnect("127.0.0.1", 6379);

    if (redis == nullptr || redis->err)
    {
        if (redis)
        {
            std::cerr << "Redis connection error: "
                      << redis->errstr << std::endl;
            redisFree(redis);
        }
        else
        {
            std::cerr << "Redis connection failed" << std::endl;
        }

        return 1;
    }

    std::cout << "Redis connected." << std::endl;

    // SCAN開始
    std::string cursor = "0";

    do
    {
        std::string pre_pattern = prefix + "*";
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(
                redis,
                "SCAN %s MATCH %s COUNT 100",
                cursor.c_str(),
                pre_pattern.c_str()
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
            std::cout << "KEY   : " << key << std::endl;            
            //std::string key_str = prefix + id;
            redisReply* del_reply = static_cast<redisReply*>(
                redisCommand(
                    redis,
                    "DEL %s",
                    key.c_str()
                )
            );
            if (del_reply == nullptr) {
                std::cerr << "DEL failed" << std::endl;
                redisFree(redis);
                return -1;
            }
            freeReplyObject(del_reply);
        }

        freeReplyObject(reply);

    } while (cursor != "0");

    // Redis切断
    redisFree(redis);

    return 0;
}