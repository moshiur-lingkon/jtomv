#ifndef __JSON_VECTOR_MAP_H__
#define __JSON_VECTOR_MAP_H__

#include <map>
#include <vector>
#include <string>

typedef long long INT64;

typedef enum {
	JSON_TYPE_ARRAY, // std::vector<Json> 
	JSON_TYPE_OBJECT, // std::map<std::string, Json> 
	JSON_TYPE_STRING, // std::string
	JSON_TYPE_BOOLEAN, 
	JSON_TYPE_INTEGER,
	JSON_TYPE_DOUBLE,
	JSON_TYPE_NULL,
	JSON_TYPE_INVALID
} JsonType;

class Json {
private:
	JsonType m_type;
	void* m_pValue;	
	int m_nPos;
	std::string m_JsonString;	

	bool ParseChar(char ch);
	bool ParseKEY_VALUE_PAIR(std::map<std::string, Json>& jsonMap);
	bool ParseLIST_OF_KEY_VALUE_PAIR(std::map<std::string, Json>& jsonMap);
	bool ParseJSON_OBJECT(Json& res);
	bool ParseSTRING(Json& res);
	bool ParseBOOLEAN(Json& res);
	bool ParseINTEGER(Json& res);
	bool ParseDOUBLE(Json& res);
	bool ParseNULL(Json& res);
	bool ParseLIST_OF_JSON(std::vector<Json>& vjson);
	bool ParseJSON_ARRAY(Json& res);
	bool ParseJSON(Json& res);
	void Clear();

public:
	virtual ~Json();
	Json();
	Json(std::string jsonString);

	JsonType GetType();
	void* GetValue();
};

#endif // __JSON_VECTOR_MAP_H__
