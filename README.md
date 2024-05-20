# QJson - A JSON parser for C++

QJson is a JSON parser for C++ that is designed to be simple and easy to use. It is a header-only library that can be included in your project by simply copying the `qjson.hpp` file into your project.

## Note
This library can only read JSON files. It cannot write to JSON files. (For now.)

## Usage

To read a JSON file:

    const qjson::Json loaded_file ("filename.json")

You can access the data by key:

    const qjson::Json loaded_file ("filename.json");

    qjson::json data = json["key"];

or by index:

    const qjson::Json loaded_file ("filename.json");

    qjson::json data = json[0];

Data can be a string, number, boolean, or another JSON object.
You can print JSON objects as long as they are a string, number, or boolean by:

    const qjson::Json loaded_file ("filename.json");

    std::cout << json["key"] << std::endl;

Data can be deleted by key:

    const qjson::Json loaded_file ("filename.json");

    json.del("key");

or by index:
    
    const qjson::Json loaded_file ("filename.json");
    
    json.del(0);