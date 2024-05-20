#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <memory>
#include <string>
#include <stack>
#include <stdexcept>

/**
 * @namespace qjson
 * Very simple JSON parser that loads JSON files into a tree structure of shared ptrs.
*/
namespace qjson {
	enum struct JsonType {
		STRING,
		OBJECT,
		ARRAY,
		UNINIT
	};

	class JsonData;
	/**
	 * @class ov_shared_ptr
	 * This is a wrapper around std::shared_ptr that allows for easier access to the underlying object. This allows for
	 * all data to be kept in shared ptrs but the user doesn't have to deal with the shared ptrs directly.
	*/
	template <class T> class ov_shared_ptr {
		public:
			ov_shared_ptr() : ptr_(std::make_shared<T>()) {}
			explicit ov_shared_ptr(JsonType type) : ptr_(std::make_shared<T>()) { ptr_->type_ = type; }
			explicit ov_shared_ptr(const std::shared_ptr<T>& ptr) : ptr_(ptr) {}
			ov_shared_ptr(std::nullptr_t ptr) : ptr_(nullptr) {} // NOLINT (explicit)

			void del(const std::string& key) {
				if (ptr_ == nullptr) {
					throw std::runtime_error("Can't delete key on null pointer. Key: " + key);
				}

				if (ptr_->type_ != JsonType::OBJECT) {
					throw std::runtime_error("Can't delete key on non-object. Key: " + key);
				}

				if (ptr_->object_data_->find(key) == ptr_->object_data_->end()) {
					throw std::runtime_error("Key " + key + " not found");
				}

				ptr_->object_data_->erase(key);
			}

			void del(const int index) {
				if (ptr_ == nullptr) {
					throw std::runtime_error("Can't delete index on null pointer. Index: " + std::to_string(index));
				}

				if (ptr_->type_ != JsonType::ARRAY) {
					throw std::runtime_error("Can't delete index on non-array. Index: " + std::to_string(index));
				}

				if (index < 0 || index >= ptr_->array_data_->size()) {
					throw std::runtime_error("Index " + std::to_string(index) + " out of bounds");
				}

				if (ptr_->array_data_->size() == 1) {
					ptr_->array_data_->clear();
				} else {
					ptr_->array_data_->erase(ptr_->array_data_->begin() + index);
				}
			}

			ov_shared_ptr<JsonData> operator[] (const std::string& key) const {
				if (ptr_ == nullptr) {
					throw std::runtime_error("Can't access key on null pointer. Key: " + key);
				}

				if (ptr_->type_ != JsonType::OBJECT) {
					throw std::runtime_error("Can't access key on non-object. Key: " + key);
				}

				if (ptr_->object_data_->find(key) == ptr_->object_data_->end()) {
					throw std::runtime_error("Key " + key + " not found");
				}

				return ptr_->object_data_->at(key);
			}

			ov_shared_ptr<JsonData> operator[] (const int index) const {
				if (ptr_ == nullptr) {
					throw std::runtime_error("Can't access index on null pointer. Index: " + std::to_string(index));
				}

				if (ptr_->type_ != JsonType::ARRAY) {
					throw std::runtime_error("Can't access index on non-array. Index: " + std::to_string(index));
				}

				if (index < 0 || index >= ptr_->array_data_->size()) {
					throw std::runtime_error("Index " + std::to_string(index) + " out of bounds");
				}

				return ptr_->array_data_->at(index);
			}

			T operator*() const {
				return *ptr_;
			}

			bool operator== (const ov_shared_ptr<T>& other) const {
				return ptr_ == other.ptr_;
			}

			bool operator!= (const ov_shared_ptr<T>& other) const {
				return ptr_ != other.ptr_;
			}

			T* operator->() const {
				return ptr_.get();
			}

			explicit operator std::string() const {
				if (ptr_ == nullptr) {
					throw std::runtime_error("Can not convert null pointer to string");
				}

				if (ptr_->type_ != JsonType::STRING) {
					throw std::runtime_error("Can not convert non-string type to string");
				}

				return ptr_->string_data_;
			}

			explicit operator T& () { return *ptr_; };
			explicit operator const T& () const { return *ptr_; }

			std::shared_ptr<T> ptr_;
	};

	inline std::ostream& operator<<(std::ostream &os, const ov_shared_ptr<JsonData>& ptr) {
		return os << std::string(ptr);
	}

	using JsonArray = std::vector<ov_shared_ptr<JsonData>>;
	using JsonObject = std::unordered_map<std::string, ov_shared_ptr<JsonData>>;

	using JsonDataPtr = ov_shared_ptr<JsonData>;
	using json = JsonDataPtr; // shorter alias for user
	using JsonArrayPtr = ov_shared_ptr<JsonArray>;
	using JsonObjectPtr = ov_shared_ptr<JsonObject>;

	/**
	 * @class JsonData
	 * JSON tree is built out of these blocks. Each block can be a string, object or array. Allows subscript access.
	*/
	class JsonData {
		public:
			std::string key_;
			JsonType type_ = JsonType::UNINIT;

			ov_shared_ptr<JsonData> operator[] (const std::string& key) const {
				if (type_ != JsonType::OBJECT) {
					throw std::runtime_error("Can't access key on non-object. Key: " + key);
				}

				if (object_data_->find(key) == object_data_->end()) {
					throw std::runtime_error("Key " + key + " not found");
				}

				return object_data_->at(key);
			}

			ov_shared_ptr<JsonData> operator[] (const int index) const {
				if (type_ != JsonType::ARRAY) {
					throw std::runtime_error("Can't access index on non-array. Index: " + std::to_string(index));
				}

				if (index < 0 || index >= array_data_->size()) {
					throw std::runtime_error("Index " + std::to_string(index) + " out of bounds");
				}

				return array_data_->at(index);
			}

			std::string string_data_;
			ov_shared_ptr<std::unordered_map<std::string, ov_shared_ptr<JsonData>>> object_data_ = nullptr;
			ov_shared_ptr<std::vector<ov_shared_ptr<JsonData>>> array_data_ = nullptr;
	};

	/**
	 * @class Json
	 * Load file in constructor and parse it into a tree structure. Access data with subscript operator.
	*/
	class Json {
		public:
			explicit Json(const std::string& filename)
				: file_ {filename},
				  raw_char_data_ {new char[buffer_size_]}
			{ parseFile(); };

			ov_shared_ptr<JsonData> operator[] (const int index) const {
				if (json_data_.array_data_ == nullptr) {
					throw std::runtime_error("JSON Parser: Can't access index on non-array");
				}

				if (index < 0 || index >= json_data_.array_data_->size()) {
					throw std::runtime_error("JSON Parser: Index " + std::to_string(index) + " out of bounds");
				}

				return json_data_.array_data_->at(index);
			}

			ov_shared_ptr<JsonData> operator[] (const std::string& key) const {
				if (json_data_.object_data_ == nullptr) {
					throw std::runtime_error("JSON Parser: Can't access key on non-object");
				}

				if (json_data_.object_data_->find(key) == json_data_.object_data_->end()) {
					throw std::runtime_error("JSON Parser: Key " + key + " not found");
				}

				return json_data_.object_data_->at(key);
			}

			~Json() = default;
		private:
			const std::streamsize buffer_size_ = 4096;

			char last_bracket_ = '\0';
			char last_symbol_ = '\0';

			std::stack<char> brackets_;
			std::stack<std::string> keys_;
			std::stack<JsonDataPtr> working_on_;

			JsonDataPtr currently_working_on_ = ov_shared_ptr<JsonData>(JsonType::ARRAY);

			std::string text_;

			bool quotes_open = false;

			bool processing_key_ = true;
			bool processing_bool_ = false;
			bool processing_number_ = false;

			std::ifstream file_;
			std::unique_ptr<char[]> raw_char_data_;

			JsonData json_data_;

			static bool isClosingBracket(const char c) { return c == '}' || c == ']'; };
			static bool isOpeningBracket(const char c) { return c == '{' || c == '['; };
			static bool isValidBracket(const char c) { return isOpeningBracket(c) || isClosingBracket(c); };

			static bool sameBracketType(const char c1, const char c2) {
				bool c1_square = (c1 == '[') || (c1 == ']');
				bool c2_square = (c2 == '[') || (c2 == ']');

				return c1_square == c2_square;
			};

			static bool isNumberPart(const char c) { return (c >= '0' && c <= '9') || c == '.' || c == '-' || c == 'e'; };

			static bool isValidNumber(const std::string& number) {
				try {
					std::stod(number);
				} catch (std::invalid_argument& e) {
					return false;
				}

				return true;
			};

			static bool isBoolPart(const char c) {
				return c == 't' || c == 'r' || c == 'u' || c == 'e' || c == 'f' || c == 'a' || c == 'l' || c == 's';
			};

			static bool isValidBool(const std::string& boolean) { return boolean == "true" || boolean == "false"; };

			bool readBuffer() {
				if (!file_) return false;
				file_.read(raw_char_data_.get(), buffer_size_);
				return true;
			};

			void parseFile() {
				while (readBuffer()) {
					std::string buffer {raw_char_data_.get()};
					parseBuffer(buffer);
				};

				if (!brackets_.empty()) {
					throw std::runtime_error("Bracket not closed: " + std::string(1, brackets_.top()));
				}

				json_data_ = *(currently_working_on_.ptr_->array_data_->at(0));
			};

			void parseBuffer(const std::string& buffer) {
				for (int i = 0; i < file_.gcount(); i++) {
					const char c = buffer[i];
					if (c == '"') {
						if (quotes_open) {
							quotes_open = false;
						} else {
							text_ = "";
							quotes_open = true;
						}
					} else {
						if (quotes_open) {
							text_ += c;
						} else {
							processJson(c);
						}
					}
				}
			};

			void processJson(const char c) {
				// ignore whitespace
				if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
					return;
				}

				// key value separator
				if (c == ':') {
					processing_key_ = false;
					keys_.push(text_);
					text_ = "";
				}

				if (isNumberPart(c) && !processing_bool_) {
					processing_number_ = true;
					text_ += c;
				}

				if (isBoolPart(c) && !processing_number_) {
					processing_bool_ = true;
					text_ += c;
				}

				if (
					(c == ',' && !isClosingBracket(last_symbol_)) // comma is only valid after a value
					|| (isClosingBracket(c) && !text_.empty()) // closing bracket shows end of value
					|| ((c == ',' || c == '}' || c == ']') && (processing_number_ || processing_bool_)) // number is finished
				) {
					if (processing_number_ && !isValidNumber(text_)) {
						throw std::runtime_error("Invalid number: " + text_);
					} else if (processing_bool_ && !isValidBool(text_)) {
						throw std::runtime_error("Invalid boolean: " + text_);
					}

					processing_number_ = false;
					processing_bool_ = false;
					if (currently_working_on_->type_ == JsonType::OBJECT) {
						if (keys_.empty()) {
							throw std::runtime_error("No key found for value");
						}

						std::string key = keys_.top();
						keys_.pop();

						auto text_data = ov_shared_ptr<JsonData>();
						text_data->type_ = JsonType::STRING;
						text_data->string_data_ = text_;

						if (currently_working_on_->object_data_ == nullptr) {
							currently_working_on_->object_data_ = ov_shared_ptr<std::unordered_map<std::string, ov_shared_ptr<JsonData>>>();
						}
						currently_working_on_->object_data_->insert({key, text_data});
					} else if (currently_working_on_->type_ == JsonType::ARRAY) {
						if (currently_working_on_->array_data_ == nullptr) {
							currently_working_on_->array_data_ = ov_shared_ptr<std::vector<ov_shared_ptr<JsonData>>>();
						}

						const auto text_data = ov_shared_ptr<JsonData>();
						text_data->type_ = JsonType::STRING;
						text_data->string_data_ = text_;

						if (currently_working_on_->array_data_ == nullptr) {
							currently_working_on_->array_data_ = ov_shared_ptr<std::vector<ov_shared_ptr<JsonData>>>();
						}
						currently_working_on_->array_data_->push_back(text_data);
					} else {
						throw std::runtime_error("Can't append value to non-object or non-array");
					}

					text_ = "";
				}

				if (isOpeningBracket(c)) {
					brackets_.push(c);
					last_bracket_ = c;

					if (currently_working_on_ != nullptr) {
						working_on_.push(currently_working_on_);
					}

					currently_working_on_ = ov_shared_ptr<JsonData>();

					if (c == '{') {
						currently_working_on_->type_ = JsonType::OBJECT;
					} else {
						currently_working_on_->type_ = JsonType::ARRAY;
					}
				} else if (isClosingBracket(c)) {
					if (brackets_.empty()) {
						throw std::runtime_error("Closing non existing bracket");
					}

					if (!sameBracketType(c, brackets_.top())) {
						throw std::runtime_error("Bracket type mismatch " + std::string(1, c) + " is closing " + std::string(1, last_bracket_));
					}

					brackets_.pop();
					last_bracket_ = c;

					if (working_on_.empty()) {
						throw std::runtime_error("Can't append value to non-object or non-array");
					}

					ov_shared_ptr<JsonData> temp = currently_working_on_;
					currently_working_on_ = working_on_.top();
					working_on_.pop();

					if (currently_working_on_->type_ == JsonType::OBJECT) {
						if (keys_.empty()) {
							throw std::runtime_error("No key found for value");
						}

						std::string key = keys_.top();
						keys_.pop();

						if (currently_working_on_->object_data_ == nullptr) {
							currently_working_on_->object_data_ = ov_shared_ptr<std::unordered_map<std::string, ov_shared_ptr<JsonData>>>();
						}

						currently_working_on_->object_data_->insert({key, temp});
					} else if (currently_working_on_->type_ == JsonType::ARRAY) {
						if (currently_working_on_->array_data_ == nullptr) {
							currently_working_on_->array_data_ = ov_shared_ptr<std::vector<ov_shared_ptr<JsonData>>>();
						}

						currently_working_on_->array_data_->push_back(temp);
					} else {
						throw std::runtime_error("Can't append value to non-object or non-array");
					}
				}

				last_symbol_ = c;
			};
	};
};