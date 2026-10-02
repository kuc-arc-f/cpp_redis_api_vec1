#include "httplib.h"
#include <iostream>
#include <cstring>
#include <future>
#include <hiredis/hiredis.h>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <uuid/uuid.h>

#include "include/models.hpp"
#include "include/DocumetDb.hpp"
using json = nlohmann::json;

std::string LOG_FILE_WRITE = "0";

static int               g_next_id = 1;
static std::mutex        g_mutex;

bool extract_bool(const std::string& json, const std::string& key, bool def = false) {
    std::string pattern = "\"" + key + "\":";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return def;
    pos += pattern.size();
    return json.substr(pos, 4) == "true";
}

// ─────────────────────────────────────────
// main
// ─────────────────────────────────────────
int main() {
    httplib::Server svr;

    svr.Post("/api/insert", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        // 1. Content-Typeの確認
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            // 2. JSONデコード (req.body をパース)
            json j = json::parse(req.body);
            std::cout << "prefix=" << req.body << "\n";

            // 3. データの取り出し (例: {"name": "Gopher", "id": 123})
            std::string prefix = j.at("prefix").get<std::string>();
            std::cout << "prefix=" << prefix << "\n";
            std::string content = j.at("content").get<std::string>();
            std::cout << "content=" << content << "\n";
            std::string vector = j.at("vector").get<std::string>();
            std::cout << "vector=" << vector << "\n";
            if( prefix.empty()){
                res.status = 400;
                res.set_content("error, prefix none", "application/json");
                return;
            }            
            DocumetDb dLib("");
            //validate
            bool ok = dLib.get_one_list(prefix, vector);
            if( ok == false){
                res.status = 400;
                res.set_content("error, vec length NG", "application/json");
                return;
            }
            dLib.vector_add(prefix, content, vector);

            NormalRespopnse re1;
            re1.ret_code = 200;
            json j1 = re1;
            std::string json_str = j1.dump();
            std::cout << json_str << std::endl;            

            res.status = 201;
            res.set_content(json_str, "application/json");
        } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
            // キーが存在しない場合など
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    });
   
    svr.Post("/api/select", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        // 1. Content-Typeの確認
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            // 2. JSONデコード (req.body をパース)
            json j = json::parse(req.body);
            // 3. データの取り出し (例: {"name": "Gopher", "id": 123})
            std::string prefix = j.at("prefix").get<std::string>();
            std::cout << "prefix=" << prefix << "\n";
            std::string vector = j.at("vector").get<std::string>();
            //std::cout << "vector=" << vector << "\n";
            int limit = j["limit"].get<int>();
            std::cout << "limit=" << limit << "\n";
            //validate
            if( prefix.empty()){
                res.status = 400;
                res.set_content("error, prefix none", "application/json");
                return;
            }               
            DocumetDb dLib("");
            bool ok = dLib.get_one_list(prefix, vector);
            if( ok == false){
                res.status = 400;
                res.set_content("error, vec length NG", "application/json");
                return;
            }
            // SCAN 実行
            auto resp =  dLib.getTableList(prefix, vector, limit);
            dLib.free_ctx();

            SearchListResp re1;
            re1.ret_code = 200;
            re1.data = resp;
            json j1 = re1;
            std::string json_str = j1.dump();
            //std::cout << json_str << std::endl;            

            res.status = 200;
            res.set_content(json_str, "application/json");
        } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
            // キーが存在しない場合など
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    });

    svr.Post("/api/delete", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        // 1. Content-Typeの確認
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            // 2. JSONデコード (req.body をパース)
            json j = json::parse(req.body);

            // 3. データの取り出し (例: {"name": "Gopher", "id": 123})
            std::string prefix = j.at("prefix").get<std::string>();
            std::cout << "prefix=" << prefix << "\n";            
            std::string id = j.at("id").get<std::string>();
            std::cout << "id=" << id << "\n";
            if( prefix.empty()){
                res.status = 400;
                res.set_content("error, prefix none", "application/json");
                return;
            } 
            DocumetDb dLib("");
            dLib.vector_delete(prefix, id);

            NormalRespopnse re1;
            re1.ret_code = 200;
            json j1 = re1; // 構造体を代入するだけ！
            std::string json_str = j1.dump();
            std::cout << json_str << std::endl;            

            res.status = 201;
            res.set_content(json_str, "application/json");
        } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
            // キーが存在しない場合など
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    });     

    // ── 起動 ────────────────────────────────

    int port_no = 8888;
    std::cout << "Server running on http://localhost:8888\n";

    svr.listen("0.0.0.0", port_no);
    return 0;
}
