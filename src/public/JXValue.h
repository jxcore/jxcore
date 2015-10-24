// Copyright & License details are available under JXCORE_LICENSE file

#include "jx/jx.h"

#include <string>
#include <vector>

namespace jxcore
{
	// c++ wrapper for JXValue you can use for accessing JavaScript values in a more convenient way
	// This is a header only class to avoid dynamic linking problems.
	class Value
	{
	public:
		Value();
		Value(JXValue &value, bool passOwnership = false);
		Value(const Value &);
		Value &operator=(const Value &);

		~Value();

		static Value eval(const std::string &expression, const std::string &scriptName = "");
		static Value array(const std::vector<Value> &values);
		static Value object(const std::map<std::string,Value> &values);
		static Value buffer(const char *buffer, size_t len);

		Value(const std::string &x);
		std::string asString() const;
		bool isString() const;

		Value(int32_t x);
		int32_t asInt() const;
		bool isInt() const;

		Value(double x);
		double asDouble() const;
		bool isDouble() const;

		bool isObject() const;
		Value operator[](const std::string &key) const;
		Value operator[](const char *key) const;
		Value operator[](const int index) const;

		bool isFunction() const;
		Value operator()() const;
		Value operator()(const Value &arg1) const;
		Value operator()(const Value &arg1, const Value &arg2) const;
		Value operator()(const Value &arg1, const Value &arg2, const Value &arg3) const;
		Value operator()(const Value &arg1, const Value &arg2, const Value &arg3, const Value &arg4) const;
		Value operator()(const std::vector<Value> &args) const;

		// if you need to go lower-level:
		JXValue jxValue() const;

	private:
		void dispose();

		mutable JXValue _value;
		bool _owned;
		int *_refCount;
	};



	inline Value::Value() :
		_owned(true),
		_refCount(new int(1))
	{
		JX_New(&_value);
		JX_SetNull(&_value);
	}

	inline Value::Value(JXValue &value, bool passOwnership) :
		_owned(passOwnership),
		_refCount(new int(1))
	{
		if (_owned) {
			// make the original pointer persistent as well
			if (!JX_IsUndefined(&value) && !JX_IsNull(&value)) {
				JX_MakePersistent(&value);
			}
		}
		_value = value;
	}

	inline Value::Value(const Value &rhs) :
		_value(rhs._value),
		_owned(rhs._owned),
		_refCount(rhs._refCount)
	{
		++(*_refCount);
	}

	inline Value & Value::operator=(const Value &rhs)
	{
		dispose();
		_refCount = rhs._refCount;
		_value = rhs._value;
		_owned = rhs._owned;
		++(*_refCount);
        return *this;
	}	

	inline Value::~Value()
	{
		dispose();
	}

	inline void Value::dispose()
	{
		if (--(*_refCount) == 0) {
			if (_owned) {
				JX_ClearPersistent(&_value);
				JX_Free(&_value);
			}
			delete _refCount;
		}
	}

	inline Value Value::eval(const std::string &expression, const std::string &scriptName)
	{
		JXValue ret;
		JX_Evaluate(expression.c_str(), scriptName.empty() ? nullptr : "", &ret);
		return Value(ret, true);		
	}

	inline Value Value::array(const std::vector<Value> &values)
	{
		JXValue ret;
		JX_CreateArrayObject(&ret);
		for (int i = 0 ; i < values.size() ; ++i) {
			JX_SetIndexedProperty(&ret, i, &values[i]._value);
		}
		return Value(ret, true);
	}

	inline Value Value::object(const std::map<std::string,Value> &values)
	{
		JXValue ret;
		JX_CreateEmptyObject(&ret);
		for (const auto &p : values) {
			JX_SetNamedProperty(&ret, p.first.c_str(), &p.second._value);
		}
		return Value(ret, true);
	}

	inline Value Value::buffer(const char *buffer, size_t len)
	{
		JXValue ret;
		JX_New(&ret);
		JX_SetBuffer(&ret, (len > 0) ? buffer : nullptr, len);
		return Value(ret, true);
	}

	inline Value::Value(const std::string &x) :
		_owned(true),
		_refCount(new int(1))
	{
		JX_New(&_value);
		JX_SetString(&_value, x.c_str(), x.size());
	}

	inline Value::Value(int32_t x) :
		_owned(true),
		_refCount(new int(1))
	{
		JX_New(&_value);
		JX_SetInt32(&_value, x);
	}

	inline Value::Value(double x) :
		_owned(true),
		_refCount(new int(1))
	{
		JX_New(&_value);
		JX_SetDouble(&_value, x);
	}

	inline std::string Value::asString() const
	{
        char *str = JX_GetString(&_value);
        std::string ret(str ? str : "");
        free(str);
        return std::move(ret);
	}

	inline bool Value::isString() const
	{
		return JX_IsString(&_value);
	}

	inline int32_t Value::asInt() const
	{
		return JX_GetInt32(&_value);
	}

	inline bool Value::isInt() const
	{
		return JX_IsInt32(&_value);
	}

	inline double Value::asDouble() const
	{
		return JX_IsDouble(&_value);
	}

	inline bool Value::isDouble() const
	{
		return JX_GetDouble(&_value);
	}

	inline bool Value::isObject() const
	{
		return JX_IsObject(&_value);
	}

	inline bool Value::isFunction() const
	{
		return JX_IsFunction(&_value);
	}

	inline Value Value::operator[](const std::string &key) const
	{
		return operator[](key.c_str());
	}

	inline Value Value::operator[](const char *key) const
	{
		JXValue ret;
		JX_GetNamedProperty(&_value, key, &ret);
		return Value(ret, true);
	}

	inline Value Value::operator[](const int index) const
	{
		JXValue ret;
		JX_GetIndexedProperty(&_value, index, &ret);
		return Value(ret, true);
	}

	inline Value Value::operator()() const
	{
		return operator()(std::vector<Value>());
	}

	inline Value Value::operator()(const std::vector<Value> &args) const
	{
		JXValue ret;
		JXValue jxArgs[args.size()];
		std::transform(args.begin(), args.end(), jxArgs, [](const Value &value){
			return value._value;
		});
		JX_CallFunction(&_value, jxArgs, args.size(), &ret);
		return Value(ret, true);
	}

	inline Value Value::operator()(const Value &arg1) const
	{
		return operator()(std::vector<Value>({arg1}));
	}

	inline Value Value::operator()(const Value &arg1, const Value &arg2) const
	{
		return operator()(std::vector<Value>({arg1, arg2}));
	}

	inline Value Value::operator()(const Value &arg1, const Value &arg2, const Value &arg3) const
	{
		return operator()(std::vector<Value>({arg1, arg2, arg3}));
	}

	inline Value Value::operator()(const Value &arg1, const Value &arg2, const Value &arg3, const Value &arg4) const
	{
		return operator()(std::vector<Value>({arg1, arg2, arg3, arg4}));
	}

	inline JXValue Value::jxValue() const
	{
		return _value;
	}
}
