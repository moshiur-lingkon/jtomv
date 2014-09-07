jtomv
=====

Parses Json and creates a recursive data structure that consists of STL map, vector, string, 64 bit int and double. All these types are wrapped by one class called <b>Json</b>

Example
-------
```
{ 
  "student" : [ "Tom-Riddle", 16 ],
  "is-dark-lord" : true
}
```
Will be converted to something like this:
```
Json(
  map<string, Json> {
    "student" : Json(  vector<Json> [Json("Tom-Riddle"), Json(16)]  ),
    "is-dark-lord" : Json( true )
  }
)
```

How to use:
-----------
<b>Json</b> has one constructor that takes the string to be parsed:
```CPP
Json(std::string jsonString)
```

To get the type of the object it holds, it has:
```CPP
JsonType GetType();
```
Where JsonType is:
```CPP
typedef enum {
	JSON_TYPE_VECTOR,
	JSON_TYPE_MAP,
	JSON_TYPE_STRING,
	JSON_TYPE_BOOL, 
	JSON_TYPE_INT,
	JSON_TYPE_DOUBLE,
	JSON_TYPE_NULL,
	JSON_TYPE_INVALID
} JsonType;
```
To get a pointer to the actual underlying object, it has the following 5 methods:

```CPP
	std::vector<Json>* GetVector();
	std::map<std::string, Json>* GetMap();
	std::string* GetString();
	bool* GetBool();
	INT64* GetInt();
	double* GetDouble();
```

To serialize the json into a string, it has:
```CPP
	void ToStr(std::string& str);
```

<b>If you make any valid change in the underlying object, the output of *ToStr()* will reflect it.</b>

Notes for the user:
-------------------
<ol>
<li> I wrote jtomv just because I didn't want my git profile to be empty. This means, this was not written by someone motivated to solve a problem. </li>
<li> jtomv was not designed to be fast, but it shouldn't be too slow. </li>
</ol>
