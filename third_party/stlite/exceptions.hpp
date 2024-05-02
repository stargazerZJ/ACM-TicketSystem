#ifndef SJTU_EXCEPTIONS_HPP
#define SJTU_EXCEPTIONS_HPP

#include <cstddef>
#include <cstring>
#include <string>
#include <stdexcept>

namespace sjtu {

 class exception : public std::exception {
//protected:
//	const std::string variant = "";
//	std::string detail = "";
//public:
//	exception() {}
//	exception(const exception &ec) : variant(ec.variant), detail(ec.detail) {}
//	virtual std::string what() {
//		return variant + " " + detail;
//	}
};

class index_out_of_bound : public exception {
	/* __________________________ */
};

class runtime_error : public exception {
	/* __________________________ */
};

class invalid_iterator : public exception {
	/* __________________________ */
};

class container_is_empty : public exception {
	/* __________________________ */
};

//using exception = std::runtime_error;
//using index_out_of_bound = exception;
//using runtime_error = exception;
//using invalid_iterator = exception;
//using container_is_empty = exception;

}

#endif
