#ifndef __JSON_VECTOR_MAP_H__
#define __JSON_VECTOR_MAP_H__

typedef enum {
	JSON_TYPE_ARRAY; // std::vector<Json> 
	JSON_TYPE_OBJECT; // std::map<Json> 
	JSON_TYPE_STRING; // std::string
	JSON_TYPE_BOOLEAN; 
	JSON_TYPE_INTEGER;
	JSON_TYPE_DOUBLE;
	JSON_TYPE_NULL;
	JSON_TYPE_INVALID;
} JsonType;

class Json {
private:
	JsonType m_type;
	void* m_pValue;	
	int m_nPos;

public:
	Json();
	Json(std::string jsonString);

	JsonType GetType();
	void* GetValue();
};

#endif // __JSON_VECTOR_MAP_H__
