#ifndef _INCLUDE_UTILS_COMMON_HEADER_
#define _INCLUDE_UTILS_COMMON_HEADER_

#include "service.h"

/*----- atoi function -----*/
inline bool str_to_number (bool& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (strtol(in, NULL, 10) != 0);
	return true;
}

inline bool str_to_number (char& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (char) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned char& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned char) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (short& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (short) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned short& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned short) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (int& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (int) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned int& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned int) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long) strtol(in, NULL, 10);
	return true;
}

inline bool str_to_number (unsigned long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (unsigned long) strtoul(in, NULL, 10);
	return true;
}

inline bool str_to_number (long long& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long long) strtoull(in, NULL, 10);
	return true;
}

inline bool str_to_number (float& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (float) strtof(in, NULL);
	return true;
}

inline bool str_to_number (double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (double) strtod(in, NULL);
	return true;
}

#ifdef __FreeBSD__
inline bool str_to_number (long double& out, const char *in)
{
	if (0==in || 0==in[0])	return false;

	out = (long double) strtold(in, NULL);
	return true;
}
#endif
/*----- atoi function -----*/

#ifdef __ENABLE_ITEM_GARBAGE__
#include <unordered_map>
#include <string>

template <class Garbage>
class GarbageSingleton {
public:
	static Garbage& Ref() {
		static Garbage staticObj;
		return staticObj;
	}
};

#include <fstream>

template <class ObjectType, class OffsetType = void>
class Garbage : public GarbageSingleton<Garbage<ObjectType>>{

private:
	typedef std::unordered_map<ObjectType*, void*> MapType;
	MapType _GarbageMap;
	bool _UseGarbage;
	
	template<class ...Args>
	void OnError(const Args& ...args){
		static std::ofstream stream;
		if(!stream.is_open())
			stream.open("garbage.log");
		static char errorbuf[500];
		snprintf(errorbuf, sizeof(errorbuf), args...);
		stream << errorbuf;
		stream << std::endl;
	}

public:
	Garbage() {
		_UseGarbage = true;
	}

	void RegisterNewObject(ObjectType* NewObj, OffsetType* Offset=NULL) 
	{
		if (!_UseGarbage)
			return;

		typename MapType::iterator It = _GarbageMap.find(NewObj);
		if (It != _GarbageMap.end()) {
			OnError("Instance already exists. %p ", NewObj);
			return;
		}

		_GarbageMap.insert(std::make_pair(NewObj, Offset)).first;
	}

	void UnregisterObject(ObjectType* Obj, OffsetType* Offset = NULL)
	{
		if (!_UseGarbage)
			return;

		typename MapType::iterator It = _GarbageMap.find(Obj);
		if (It == _GarbageMap.end()) {
			OnError("Instance doesn't exist. %p ", Obj);
			return;
		}

		_GarbageMap.erase(It);
	}

	bool VerifyObject(ObjectType* Obj, OffsetType* Offset, const std::string& func, int line) {
		if (!_UseGarbage)
			return true;

		typename MapType::iterator It = _GarbageMap.find(Obj);
		if (It == _GarbageMap.end()) {
			OnError("Verify of %p failed.  func %s  line %d", Obj, func.c_str(), line);
			return false;
		}

		if (Offset != NULL && It->second != Offset) {
			OnError("Verify of %p value failed. r(%p) v(%p) func %s  line %d", Obj, Offset, func.c_str(), line);
			return false;
		}

		return true;
	}

	void DisableGarbage() {
		_UseGarbage = false;
	}

	std::string GetLogMessage() {
		if (!_UseGarbage)
			return "Garbage is disabled.";

		std::string res = "Garbage contains ";
		res += std::to_string(_GarbageMap.size());
		res += " Elements";

		if (_GarbageMap.empty())
			res += "(something goes wrong)";
		else
			res += "(the debugging proceed)";
		return res;
	}
};



#endif

#endif