/*
 * Copyright (C) 2015 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * cojson.hpp - main header file
 *
 * This file is part of COJSON Library. http://hutorny.in.ua/projects/cojson
 *
 * The COJSON Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License v2
 * as published by the Free Software Foundation;
 *
 * The COJSON Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the COJSON Library; if not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>.
 */

#ifndef COJSON_HPP_
#define COJSON_HPP_
#ifndef COJSON_HELPERS_HPP_
#	include "cojson_helpers.hpp"
#endif
#pragma GCC diagnostic ignored "-Wattributes"

namespace cojson {
	/**
	 * Default cojson configuration
	 */
	struct default_config {
		/** character type: char, wchar_t or char16_t						*/
		typedef char char_t;

		/** controls where the constant strings are retrieved from */
		static constexpr enum class cstring_is {
			const_char, /** default, as provided by compiler  				*/
			avr_progmem	/** from the program memory.
			Note! This option for AVR only, all strings are expected to be
			properly placed in progmem with __attribute__((progmem))		*/
		} cstring = cstring_is::const_char;

		/** controls behavior on integral overflow 							*/
		static constexpr enum class overflow_is {
			ignored, 	/** overflow condition is silently ignored			*/
			error, 		/** overflow causes an error						*/
			saturated	/**	numbers are saturated on overflow				*/
		} overflow = overflow_is::ignored;

		/** controls implementation of iostate::error						*/
		static constexpr enum class iostate_is {
			_notvirtual,/** iostate::error is implemented as non-virtual	*/
			_virtual,	/** iostate::error is implemented as virtual		*/
		} iostate = (sizeof(double) == sizeof(float)) ?
				iostate_is::_notvirtual : iostate_is::_virtual;

		/** controls behavior on read encountered element mismatching
		 *	targed data type												*/
		static constexpr enum class mismatch_is {
			skipped,
			error
		} mismatch = mismatch_is::skipped;

		/** controls default null handling.
		 *  could be overridden in a custom accessor						*/
		static constexpr enum class null_is {
			skipped, 	/** overflow condition is silently ignored			*/
			error, 		/** overflow causes an error						*/
		} null = null_is::skipped;

		/** controls implementation of temporary buffer, used for reading
		 * 	names and writing floating data types							*/
		static constexpr enum class temporary_is {
			_static,	/**	temporary buffer preallocated in static data	*/
			_automatic	/**	temporary buffer allocated on the stack 		*/
		} temporary = (sizeof(double) < 8) ?
		 /* by default temporary buffer is static on low end CPU (AVR 8 bit).
		  * sizeof(double) is used as indication of lowendness				*/
			temporary_is::_static : temporary_is::_automatic;

		/** controls size of temporary buffer								*/
		static constexpr unsigned temporary_size = 24; /* double should fit */

		static constexpr enum class write_double_impl_is {
			internal,
			with_sprintf,
			external
		} write_double_impl = write_double_impl_is::internal;
		using write_double_integral_type = uint32_t;
		static constexpr unsigned write_double_precision = 6;
	private:
		default_config();
	};


	struct config : default_config {
	/************************************************************************/
	/* configuration is externalized for the purpose of separate versioning	*/
#		include <cojson.config>
	/************************************************************************/
	public:
		static constexpr bool null_is_error = null == null_is::error;
	private:
		config();
	};


typedef config::char_t char_t;
typedef unsigned size_t; /* unsigned char may save up to 500 bytes on avr */

/* void type and value to be used in templates instead of destination class */
const struct void_t {} void_v {};

/*
 * Data accessors - wrap access to data into a uniform interface
 */
namespace accessor {

/**
 * Accessor for a static variable via pointer
 */
template<typename T, T* P>
struct pointer {
	typedef T clas;
	typedef T type;
	static constexpr bool canget = true;
	static constexpr bool canset = true;
	static constexpr bool canlref   = true;
	static constexpr bool canrref   = true;
	static constexpr bool is_vector = false;
	static inline bool has() noexcept { return true; }
	static inline T& lref() noexcept { return *P; }
	static inline const T& rref() noexcept { return *P; }
	static inline T get() noexcept { return *P; }
	static inline void set(const T& v) noexcept {
		*P = v;
	}
	static inline constexpr bool null(void_t) noexcept {
		return not config::null_is_error;
	}
	static inline void init(T&) noexcept { }
private:
	pointer();
};


/**
 * Accessor for a (static) variable via a function returning reference
 */
template<typename T, T& (*G)() noexcept>
struct reference {
	typedef T clas;
	typedef T type;
	/* readable/writeable not used to avoid confusion */
	static constexpr bool canget = true;
	static constexpr bool canset = true;
	static constexpr bool canlref= true;
	static constexpr bool canrref   = true;
	static constexpr bool is_vector = false;
	/** should return false if get is not applicable */
	static inline constexpr bool has() noexcept { return true; }
	static inline T get() noexcept { return G(); }
	static inline T& lref() noexcept { return G(); }
	static inline const T& rref() noexcept { return G(); }
	static inline void set(const T& v) noexcept { G() = v; }
	static inline constexpr bool null(void_t) noexcept {
		return not config::null_is_error;
	}
	static inline void init(T&) noexcept { }
private:
	reference();
};

/**
 * Accessor for a (static) variable via a function returning pointer
 */
template<typename T, T* (*G)() noexcept>
struct function {
	typedef T clas;
	typedef T type;
	static constexpr bool canget = true;
	static constexpr bool canset = true;
	static constexpr bool canlref   = true;
	static constexpr bool canrref   = true;
	static constexpr bool is_vector = false;
	static inline bool has() noexcept { return G() != nullptr; }
	static inline T& lref() noexcept { return *G(); }
	static inline const T& rref() noexcept { return *G(); }
	static inline T get() noexcept { return *G(); }
	static inline void set(const T& v) noexcept {
		T* p = G();
		if( p != nullptr ) *p = v;
	}
	static inline constexpr bool null(void_t) noexcept {
		return not config::null_is_error;
	}
	static inline void init(T&) noexcept { }
private:
	function();
};



/**
 * Field accessor via member pointer
 */

template<class C, typename T, T C::*V>
struct field {
	typedef C clas;
	typedef T type;
	static constexpr bool canget = true;
	static constexpr bool canset = true;
	static constexpr bool canlref   = true;
	static constexpr bool canrref   = true;
	static constexpr bool is_vector = false;
	static inline constexpr bool has() noexcept { return true; }
	static inline void init(T&) noexcept { }
	static inline T get(const C& o) noexcept { return o.*V; }
	static inline T& lref(C& o) noexcept { return o.*V; }
	static inline const T& rref(const C& o) noexcept { return o.*V; }
	static inline void set(C& o, T v) noexcept { o.*V = v; }
	static inline constexpr bool null(C&) noexcept {
		return not config::null_is_error;
	}

private:
	field();
};

/**
 * Field accessor via class methods
 */
template<class C, typename T, T (C::*G)() const noexcept,
		void (C::*S)(T) noexcept>
struct methods {
	typedef C clas;
	typedef T type;
	static constexpr bool canget = true;
	static constexpr bool canset = true;
	static constexpr bool canlref= false;
	static constexpr bool canrref= false;
	static constexpr bool is_vector = false;
	static inline constexpr bool has() noexcept { return true; }
	static inline T get(const C& o) noexcept { return (o.*G)(); }
	static T& lref(const C& o) noexcept; 		/* not possible */
	static const T& rref(const C&) noexcept;	/* not possible */
	static inline void set(C& o, const T& v) noexcept { (o.*S)(v); }
	static inline void init(T&) noexcept { }
	static inline constexpr bool null(C&) noexcept {
		return not config::null_is_error;
	}
private:
	methods();
};

/**
 * Vector item accessor via function returning pointer
 */
template<typename T, T* (*V)(size_t) noexcept>
struct vector {
	typedef T clas;
	typedef T type;
	static constexpr bool canget = true; /* array is accessible for reading */
	static constexpr bool canset = true;
	static constexpr bool canlref   = true;
	static constexpr bool canrref   = true;
	static constexpr bool is_vector = true;
	static inline bool has(size_t i) noexcept { return V(i) != nullptr; }
	static inline const T get(size_t i) noexcept { return *V(i); }
	static inline T& lref(size_t i) noexcept { return *V(i); }
	static inline const T& rref(size_t i) noexcept { return *V(i); }
	static inline void set(size_t i, const T & v) noexcept { *V(i) = v; }
	//static inline void init(size_t,T &) noexcept { }
	static inline void init(T&) noexcept {}
	static inline constexpr bool null(void_t) noexcept {
		return not config::null_is_error;
	}
private:
	vector();
};

/**
 * Array accessor
 */
template<typename T, size_t N, T (&A)[N]>
struct array {
	typedef T clas;
	typedef T type;
	static constexpr bool canget = true;
	static constexpr bool canset = true;
	static constexpr bool canlref   = true;
	static constexpr bool canrref   = true;
	static constexpr bool is_vector = true;
	static inline bool has(size_t i) noexcept { return i < N; }
	static inline const T get(size_t i) noexcept { return A[i]; }
	static inline T& lref(size_t i) noexcept { return A[i]; }
	static inline const T& rref(size_t i) noexcept { return A[i]; }
	static inline void set(size_t i, const T & v) noexcept { A[i] = v; }
	static inline void init(T&) noexcept {}
	static inline constexpr bool null(void_t) noexcept {
		return not config::null_is_error;
	}
private:
	array();
};


/**
 * Accessor for a (static) variable via a pair of functions
 */
template<typename T, T (*G)() noexcept, void (*S)(T) noexcept>
struct functions {
	typedef void clas;
	typedef T type;
	static constexpr bool canget = true;
	static constexpr bool canset = true;
	static constexpr bool canlref   = false;
	static constexpr bool canrref   = false;
	static constexpr bool is_vector = false;
	static inline constexpr bool has() noexcept { return true; }
	static inline const T get() noexcept { return G(); }
	static const T& rref() noexcept;
	static T& lref() noexcept;
	static inline void set(const T& v) noexcept { S(v); }
	static inline void init(T&) noexcept { }
	static inline constexpr bool null(void_t) noexcept {
		return not config::null_is_error;
	}
private:
	functions();
};
} /* namespace accessor */


namespace details {
/** JSON char traits. returned by lexer and used by readers */
static constexpr int bit(int N) noexcept { return 1 << N; }

typedef std::conditional<config::cstring == config::cstring_is::avr_progmem,
		progmem<char_t>, const char_t*>::type cstring;

typedef std::conditional<config::cstring == config::cstring_is::avr_progmem,
		progmem<char_t>, char_t>::type char_l;

/******************************************************************************/
template<typename T>
struct literal_strings {
};

template<>
struct literal_strings<char> {
	static inline constexpr const char * null_l()  noexcept { return "null"; }
	static inline constexpr const char * true_l()  noexcept { return "true"; }
	static inline constexpr const char * false_l() noexcept { return "false"; }
	static inline constexpr const char * bom() noexcept {return "\xEF\xBB\xBF";}
	/*	https://tools.ietf.org/html/rfc7159#section-8.1
	 *	Implementations MUST NOT add a byte order mark to the beginning of a
	 *	JSON text.  In the interests of interoperability, implementations
	 *	that parse JSON texts MAY ignore the presence of a byte order mark
	 *	rather than treating it as an error.								*/
};

template<>
struct literal_strings<char16_t> {
	static inline constexpr const char16_t * null_l()noexcept{return u"null";  }
	static inline constexpr const char16_t * true_l()noexcept{return u"true";  }
	static inline constexpr const char16_t *false_l()noexcept{return u"false"; }
	static inline constexpr const char16_t * bom   ()noexcept{return u"\xFEFF";}
};

template<>
struct literal_strings<char32_t> {
	static inline constexpr const char32_t * null_l()noexcept{return U"null";  }
	static inline constexpr const char32_t * true_l()noexcept{return U"true";  }
	static inline constexpr const char32_t *false_l()noexcept{return U"false"; }
	static inline constexpr const char32_t * bom   ()noexcept{return U"\xFEFF";}
};

template<>
struct literal_strings<wchar_t> {
	static inline constexpr const wchar_t * null_l()noexcept{ return L"null";  }
	static inline constexpr const wchar_t * true_l()noexcept{ return L"true";  }
	static inline constexpr const wchar_t *false_l()noexcept{ return L"false"; }
	static inline constexpr const wchar_t * bom   ()noexcept{ return L"\xFEFF";}
};

template<>
struct literal_strings<progmem<char>> {
private:
	static constexpr const char _null_l[] = "null";
	static constexpr const char _true_l[] = "true";
	static constexpr const char _false_l[]= "false";
	static constexpr const char _bom[]    = "\xEF\xBB\xBF";
public:
	static inline constexpr progmem<char> null_l() noexcept { return progmem<char>( _null_l ); };
	static inline constexpr progmem<char> true_l() noexcept { return progmem<char>(  _true_l ); }
	static inline constexpr progmem<char> false_l() noexcept { return progmem<char>(  _false_l ); }
	static inline constexpr progmem<char> bom() noexcept { return progmem<char>(  _bom ); }
};


/**
 * list of JSON literals.
 */
struct literal : literal_strings<char_l> {
	//https://tools.ietf.org/html/rfc7159#section-2
	static constexpr char_t begin_array 	= '['; /** [ left square bracket  */
	static constexpr char_t begin_object    = '{'; /** { left curly bracket   */
	static constexpr char_t end_array       = ']'; /** ] right square bracket */
	static constexpr char_t end_object      = '}'; /** } right curly bracket  */
	static constexpr char_t quotation_mark  = '"'; /** " quotation mark 	  */
	static constexpr char_t name_separator  = ':'; /** : colon 				  */
	static constexpr char_t value_separator = ','; /** , comma 	 			  */
	static constexpr char_t minus			= '-'; /** - minus sign 		  */
	static constexpr char_t plus			= '+'; /** - minus sign 		  */
	static constexpr char_t digit0			= '0'; /** 0 					  */
	static constexpr char_t decimal			= '.'; /** . decimal point 		  */
	static constexpr char_t escape 			= '\\';/**  \ reverse solidus 	  */
	static constexpr char_t hex_mark 		= 'u'; /** u char code prefix	  */
	static constexpr char_t digitA			= 'A'; /** A 					  */
	static constexpr char_t digita			= 'a'; /** A 					  */
	static constexpr char_t ws 				= ' ';

	/* the characters that must be escaped:
	 * quotation mark, reverse solidus,
	 * and the control characters (U+0000 through U+001F).
	 */
	static constexpr char_t escaped[] = {
        '"', 		// quotation mark  U+0022
		'\\' 		// reverse solidus U+005C
					// control characters not listed by intent
	};

	static inline constexpr bool is_escaped(char_t c) {
		return c == escaped[0] || c == escaped[1];
	}

	/* common escape sequences
	 */
	static constexpr char_t common[] = {
		0x08,		// b    backspace       U+0008
		0x0C,		// f    form feed       U+000C
		0x0A,		// n    line feed       U+000A
		0x0D,		// r    carriage return U+000D
		0x09		// t    tab             U+0009
	};


	static constexpr char_t replacement[] = {
		'b',		// b    backspace       U+0008
		'f',		// f    form feed       U+000C
		'n',		// n    line feed       U+000A
		'r',		// r    carriage return U+000D
		't' 		// t    tab             U+0009
	};

	static inline constexpr char_t replace_common(char_t c) {
		return
			c == common[0] ? replacement[0]
		  : c == common[1] ? replacement[1]
		  :	c == common[2] ? replacement[2]
		  : c == common[3] ? replacement[3]
		  : c == common[4] ? replacement[4]
		  : c;
	}

	static inline constexpr bool is_control(char_t c) {
		return c < ws;
	}
private:
	literal();
};
/******************************************************************************/

class noncopyable {
private:
	noncopyable(const noncopyable&);
	noncopyable& operator=(const noncopyable&);
public:
	noncopyable() { }
};

struct value;
struct member;

/**
 * unnamed element
 */
typedef const value&  (*item)();

/**
 * named element
 */
typedef const member& (*node)();

/**
 * name type - a function returning pointer to a name
 */
typedef cstring (*name)();

/**
 * Error codes
 */
enum class error_t : unsigned char {
	noerror  = 0x00, /* no error 											*/
	notfound = 0x01, /* member not found 									*/
	mismatch = 0x02, /* data type mismatch									*/
	overflow = 0x04, /* integer overflow 									*/
	noobject = 0x08, /* destination object was not available				*/
	overrun  = 0x10, /* buffer overrun 							  			*/
	bad		 = 0x20, /* bad character or malformed data 					*/
	eof		 = 0x40, /* end of file indication		  						*/
	ioerror	 = 0x80, /* I/O error while reading data (set inside the stream	*/
	failed	 = ioerror | bad,
	blocked	 = ioerror | bad | mismatch
};

static inline constexpr error_t operator&(error_t a, error_t b) noexcept {
    return static_cast<error_t>(
    	static_cast<unsigned char>(a) & static_cast<unsigned char>(b));
}

static inline constexpr error_t operator|(error_t a, error_t b) noexcept {
    return static_cast<error_t>(
    	static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
}

static inline constexpr error_t operator^(error_t a, error_t b) noexcept {
    return static_cast<error_t>(
    	static_cast<unsigned char>(a) ^ static_cast<unsigned char>(b));
}

static inline constexpr error_t operator~(error_t a) noexcept {
    return static_cast<error_t>(~static_cast<unsigned char>(a));
}

static inline error_t & operator|=(error_t & a, error_t b) noexcept {
	*reinterpret_cast<unsigned char*>(&a) |= static_cast<unsigned char>(b);
    return a;
}

static inline constexpr unsigned char operator+(error_t v) noexcept {
	return static_cast<unsigned char>(v);
}

template<config::iostate_is V = config::iostate>
class iostate_t;

/**
 * iostate with virtual methods
 */
template<>
struct iostate_t<config::iostate_is::_virtual> : noncopyable {
	static constexpr bool isvirtual = true;
	virtual void error(error_t e) noexcept { err |= e; }
	virtual error_t error() const noexcept { return err; }
	virtual void clear() noexcept { err = error_t::noerror; }
private:
	error_t err;
};


/**
 * iostate with inline methods
 */
template<>
struct iostate_t<config::iostate_is::_notvirtual> : noncopyable {
	static constexpr bool isvirtual = false;
	inline void error(error_t e) noexcept { err |= e; }
	inline error_t error() const noexcept { return err; }
	inline void clear() noexcept { err = error_t::noerror; }
private:
	error_t err;
};

/**
 * iostate selector per configuration
 */
struct iostate : iostate_t<> {
	static constexpr char_t eos_c = -1; /* end of stream character */
	static constexpr char_t err_c = -2; /* i/o error character */
	inline bool isgood() const noexcept {
		return (error() & error_t::failed) == error_t::noerror;
	}
	inline bool eof() const noexcept {
		return (error() & error_t::eof) != error_t::noerror;
	}
	template<typename T>
	static inline constexpr bool isok(T chr) noexcept {
		using sT = typename std::make_signed<T>::type;
		return static_cast<sT>(chr) >= 0;
	}
};
/**
 * Input stream interface
 */
struct istream : virtual iostate {
	/**
	 * reads a single character from the stream.
	 * places it in the dst and advances head to the next position
	 * returns true on success or false on error
	 * in latter case dst holds error code (fail or eof)
	 */
	virtual bool get(char_t& dst) noexcept = 0;
};

/**
 * Output stream interface
 */

struct ostream : virtual iostate {
	/**
	 * writes a single character to the stream.
	 * returns true on success or false on error
	 */
	virtual bool put(char_t c) noexcept = 0;
	/**
	 * writes a zero-terminated string to the stream.
	 * returns true on success or false on error
	 */
	inline bool put(const char_t* s) noexcept { return puts(s); }
	bool puts(const char_t* s) noexcept;
	template<class C>
	bool puts(C s) noexcept;
};

template<>
bool ostream::puts<progmem<char>>(progmem<char>) noexcept;

template<>
inline bool ostream::puts<char_t*>(char_t* v) noexcept {
	return puts(const_cast<const char_t*>(v));
}

template<size_t N, config::temporary_is = config::temporary>
struct temporary_s {
	static constexpr size_t size = N;
	inline operator char_t*() noexcept { return buffer; }
	inline operator const char_t*() const noexcept { return buffer; }
	char_t buffer[N];
};

template<size_t N>
struct temporary_s<N, config::temporary_is::_static> {
	static constexpr size_t size = N;
	inline operator char_t*() noexcept { return buffer; }
	inline operator const char_t*() const noexcept { return buffer; }
	static char_t buffer[N];
};

template<size_t N>
char_t temporary_s<N,config::temporary_is::_static>::buffer[N];

struct temporary : temporary_s<config::temporary_size> {};

enum class ctype : int {
	unknown		= 0,
	/* type bits											*/
	whitespace	= bit( 0), /* 0x9 0xA, 0xD, 0x20			*/
	delim		= bit( 1), /* }], \t\n\r					*/
	string		= bit( 2), /* any character > 0x20			*/
	special		= bit( 3), /* btnfru"\						*/
	value		= bit( 4), /* tfn-0123456789{["				*/
	null		= bit( 5), /* null							*/
	boolean		= bit( 6), /* true false					*/
	digit		= bit( 7), /* 0123456789					*/
	sign		= bit( 8), /* +-							*/
	decimal		= bit( 9), /* .								*/
	exponent	= bit(10), /* eE							*/
	array		= bit(11), /* [,]							*/
	object		= bit(12), /* {,}							*/
	hex			= bit(13), /* abcdef						*/
	heX			= bit(14), /* ABCDEF						*/
	/*reserved	  bit(15)	  for error indication			*/
	/* common masks											*/
	literal		= null | boolean,
	number		= digit | sign | decimal | exponent,
	numeric		= number | delim,
	unhex		= digit | hex | heX,
	unescape	= special,
	arraynull 	= array | null,
	objectnull	= object | null,
	stringnull	= string | null,
	/* error codes 											*/
	eof			= -1,
	err			= -2
};

ctype chartype(char_t) noexcept;

static inline constexpr int operator+(ctype v) noexcept {
	return static_cast<int>(v);
}

static inline constexpr ctype operator&(ctype a, ctype b) noexcept {
	return static_cast<ctype>(static_cast<int>(a) & static_cast<int>(b));
}

static inline void operator&=(ctype& a, int b) noexcept {
	*reinterpret_cast<int*>(&a) &= b;
}

static inline constexpr ctype operator|(ctype a, ctype b) noexcept {
	return static_cast<ctype>(static_cast<int>(a) | static_cast<int>(b));
}

static inline constexpr bool isvalid(ctype ct) noexcept {
	return ct > ctype::unknown;
}

static inline constexpr bool hasbits(ctype ct, ctype mask) noexcept {
	return ct > ctype::unknown ?
		(static_cast<int>(ct) & static_cast<int>(mask)) : false;
}

static inline constexpr ctype andmask(ctype ct, ctype mask) noexcept {
	return ct <= ctype::unknown ? ct : (ct & mask);
}

static inline /*constexpr*/ bool isws(char_t chr) noexcept {
	return hasbits(chartype(chr), ctype::whitespace);
}

/**
 * Lexer/scanner
 */
struct lexer : noncopyable {
	inline lexer(istream& in) noexcept : stream(in), hold(0) {}

	static inline void char_typify(
		void (*add)(const char * str,ctype traits)noexcept) noexcept {
		add("\t\n\r ",		ctype::whitespace);
		add("btfnru\"\\",	ctype::special);
		//add("\"\\",			ctype::escaped);
		add("tfn-0123456789{[\"", ctype::value);
		add("true", 		ctype::boolean);
		add("false", 		ctype::boolean);
		add("null", 		ctype::null);
		add("0123456789",	ctype::digit);
		add("-+", 			ctype::sign);
		add(".", 			ctype::decimal);
		add("eE", 			ctype::exponent);
		add("}],\t\n\r ",	ctype::delim);
		add("[,]",			ctype::array);
		add("{,}",			ctype::object);
		add("abcdef", 		ctype::hex);
		add("ABCDEF", 		ctype::heX);
	}

	/** skips BOM if available, returns first BOM character or 0 if no BOM
	 * or eof on error or end of file
	 */

	char_t skip_bom() noexcept;
	/** returns true if current position in stream contains character
	 * of expected type 													*/
	ctype value(ctype expected) noexcept;
	/** reads array markup, returns returns char type read					*/

	/** reads string markup, returns true on success						*/
	ctype string(char_t& dst, bool first) noexcept;
	/** scans number, returns a type of symbol read							*/

	inline ctype get(char_t& dst, ctype mask) noexcept {
		return andmask(get(dst),mask);
	}

	inline ctype skip(char_t& dst, ctype mask) noexcept {
		ctype ct;
		while( hasbits(ct=get(dst), mask) );
		return ct;
	}
	inline bool skip(ctype mask) noexcept {
		char_t tmp;
		ctype ct = skip(tmp, mask);
		if( ct > ctype::unknown ) {
			back(tmp);
			return true;
		}
		return ct >= ctype::eof;
	}
	inline bool skipws(char_t& dst) noexcept {
		return isvalid(skip(dst, ctype::whitespace));
	}

	/** reads member, returns ctype::cstring on success						*/
	bool member(char_t*& l) noexcept;
	/** skips one or more elements, returns true on success */
	bool skip(bool list=false) noexcept;
	/** skips string or remainder of such 									*/
	bool skip_string(bool first) noexcept;
	inline void error(error_t e) noexcept { stream.error(e); }
	inline error_t error() const noexcept {
		/* eof is not a lexer error */
		return	static_cast<error_t>(stream.error() & ~error_t::eof);
	}

	inline ctype mismatch() noexcept {
		error(error_t::mismatch);
		return ctype::unknown;
	}

	inline ctype bad() noexcept {
		error(error_t::bad);
		return ctype::unknown;
	}

	inline ctype bad(char_t c) noexcept {
		return iostate::isok(c) ? bad() : eos2eof(c);
	}

	static constexpr inline ctype eos2eof(char_t c) noexcept {
		return c == iostate::eos_c ? ctype::eof : ctype::err;
	}

	inline void restart() noexcept {
		hold = 0;
	}

	inline void back(char_t chr) noexcept {
		hold = chr;
	}

	static constexpr bool is_null(char_t chr) noexcept {
		return chr == literal_strings<char_t>::null_l()[0];
	}

private:
	ctype unescape(char_t& chr ) noexcept;
	ctype unhex(char_t& chr) noexcept;
	ctype get(char_t& dst) noexcept;
	bool skip_member(bool first) noexcept;
	bool literal(cstring) noexcept;
	static inline constexpr bool is_valid(int ct) noexcept {
		return cojson::details::isvalid(static_cast<ctype>(ct));
	}

private:
	istream& stream;
	temporary name;
	char_t hold;
};

/******************************************************************************/
/* multiplication by 10 with saturation on overflow */
template<typename T>
static inline bool tenfold(T& val, T digit) noexcept {
	static constexpr T max = std::numeric_limits<T>::max() / 10;
	static constexpr T mxd = std::numeric_limits<T>::max() % 10;
	static constexpr T min = std::numeric_limits<T>::min() / 10;
	static constexpr T mnd = std::numeric_limits<T>::min() % 10;
	static constexpr bool overflow_check =
			config::overflow != config::overflow_is::ignored;
	static constexpr bool saturation  =
			config::overflow == config::overflow_is::saturated;
	if( overflow_check && std::is_signed<T>::value ) {
		if( val < min || (val == min && digit < mnd) ) {
			val = std::numeric_limits<T>::min();
			return saturation;
		}
	}
	if( overflow_check ) {
		if( val > max || (val == max && digit > mxd) ) {
			val = std::numeric_limits<T>::max();
			return saturation;
		}
	}
	val *= 10;
	val += digit;
	return true;
}

/******************************************************************************/
/* JSON readers																  */

template<typename T>
struct reader {
	/**
	 * Reads value of given type from the input stream.
	 * Type of the value is expected to match data type
	 * if it can read only part of the value it skips the remainder
	 */
	static bool read(T& val, lexer& in) noexcept {
		/* routing read of types shorter than int to reader<int>
		 * could save ~50 bytes per type on avr, if overflow control is not set.
		 * decision is made to keep type-specific reads
		 */
		char_t digit = 0;
		signed char sign = 0;
		val = 0;
		ctype ct;
		if( ! isvalid(in.value(ctype::numeric)) ) return false;
		while(true) switch( ct = in.get(digit, ctype::numeric) ) {
		default:
			in.error(error_t::mismatch);
			return false;
		case ctype::unknown:
			in.error(error_t::bad);
			return false;
		case ctype::delim:
			if( !isws(digit) ) in.back(digit);
			/* no break */
		case ctype::eof:
			return true;
		case ctype::digit:
			digit -= literal::digit0;
			if( ! sign ) sign = 1;
			if( tenfold<T>(val, (sign > 0 ? digit : -digit)) ) continue;
			in.error(error_t::overflow);
			return in.skip(ctype::number);
		case ctype::sign:
			if( std::is_signed<T>::value && digit == literal::minus ) {
				if(  sign ) {
					in.error(error_t::bad);
					return false;
				}
				sign = -1;
				continue;
			}
			in.error(digit == literal::plus || sign ?
					error_t::bad : error_t::mismatch);
			return false; //in.skip(ctype::number);
		};
		return true;
	}
};

template<>
struct reader<char_t*> {
	static bool read(char_t* dst, size_t n, lexer& in) noexcept;
};

template<>
struct reader<double> {
	static bool read(double&, lexer&) noexcept;
};

template<>
struct reader<float> {
	static inline bool read(float& val, lexer& in) noexcept {
		double tmp = 0;
		if( ! reader<double>::read(tmp,in) ) return false;
		val = tmp;
		return true;
	}
};

template<>
struct reader<bool> {
	static bool read(bool& val, lexer& in) noexcept {
		ctype ct;
		if( ! isvalid(ct=in.value(ctype::boolean) )) return false;
		val = ct == (ctype::boolean | ctype::value);
		return true;
	}
};

/**
 * helper for soft dependency on sprintf(...double);
 */
template<typename C, typename T>
bool gfmt(C,size_t,T) noexcept;

template<>
bool gfmt<char_t*, double>(char_t*, size_t, double) noexcept;

template<>
inline bool gfmt<char_t*, float>(char_t* b, size_t s, float v) noexcept {
	return gfmt<char_t*, double>(b,s,v);
}

template<typename T>
bool write_number(T val, bool negative, T divider, ostream& out) noexcept {
	bool was = false;
	if( negative && ! out.put(literal::minus) ) return false;
	while( divider ) {
		T digit = (val / divider) % 10;
		divider /= 10;
		if( digit || was || ! divider) {
			if( ! out.put(literal::digit0 + digit) ) return false;
			was = true;
		}

	}
	return true;
} /* avr: 152 bytes for int, 748 bytes for long */

/******************************************************************************/
/* JSON writers																  */
template<typename T>
struct writer {
	/** write single value. Default implementation for integral numbers */
	static bool write(const T& val, ostream& out) noexcept {
		static_assert(std::is_integral<T>::value,
			"Default writer implementation supports integral types only");
		typedef numeric_helper<T> H;
		typedef typename H::U U;;
		return write_number<U>(H::abs(val),H::is_negative(val), H::pot, out);
	}
};

template<>
struct writer<const char_t*> {
	static bool write(const char_t*, ostream&) noexcept;
	static bool write(char_t, ostream&) noexcept;
};

template<>
struct writer<progmem<char_t>> {
	static bool write(progmem<char_t> s, ostream& o) noexcept;
};

template<>
struct writer<char_t*> {
	static inline bool write(char_t* str, ostream& out) noexcept  {
		return writer<const char_t*>::write(str,out);
	}
};

template<>
struct writer<double> {
	static bool write(const double& val, ostream& out) noexcept;
};

template<>
struct writer<float> {
	static inline bool write(const double val, ostream& out) noexcept {
		return writer<double>::write(val,out);
	}
};

template<>
struct writer<bool> {
	static inline bool write(bool val, ostream& out) noexcept {
		return out.puts(val ? literal::true_l() : literal::false_l());
	}
};

/**
 * helper for getting array extent
 */
template<class C, typename T, size_t N>
static inline constexpr size_t countof(T (C::*)[N]) noexcept { return N; }

/**
 * helper for getting array extent
 */
template<typename T, size_t N>
static inline constexpr size_t countof(T (&)[N]) noexcept  { return N; }

} /* namespace details */
using lexer = details::lexer;
using cstring = details::cstring;

namespace details {
/**
 * Generic JSON value
 */
struct value : noncopyable {
	virtual bool read(lexer&) const noexcept = 0;
	virtual bool write(ostream&) const noexcept = 0;
	static bool null(ostream& out) noexcept;
protected:
	template<class C> friend class collection;
	/* default null handling - ignore or abort per configuration */
	template<class T>
	static inline constexpr bool null(const T&) noexcept {
		return not config::null_is_error;
	}
};

/**
 * Array iterator
 */
struct iterator {
	static constexpr auto start = ctype::arraynull;
	static constexpr auto finish = literal::end_array;
	static constexpr auto middle = ctype::array;
	static constexpr bool skiplist = true;
	static inline constexpr bool prolog(lexer&) noexcept {
		return true;
	}
	inline size_t operator++(int) noexcept { return curr++; }
	inline operator size_t() const noexcept { return curr; }
	size_t curr = 0;
};

/**
 * Object indexer
 */

struct indexer {
	static constexpr auto start = ctype::objectnull;
	static constexpr auto finish = literal::end_object;
	static constexpr auto middle = ctype::object;
	static constexpr bool skiplist = false;
	inline bool prolog(lexer& in) noexcept {
		return in.member(curr);
	}
	inline const char_t * operator++(int) noexcept { return curr; }
	inline operator const char_t *() const noexcept { return curr; }
	char_t* curr = nullptr;
};

/**
 * implements algorithm for reading arrays and objects
 * I - item iterator (iterator for arrays, indexer for objects)
 * S - structural object (array, object, etc)
 * C - destination object
 */

template<class I = iterator>
struct collection {
	/*
	 * Several objects collaborate for reading collections:
	 * 1. structural object that knows how to read an item
	 * 2. agent that knows how JSON is made for the structural object
	 * 3. agent that knows what type of index needed
	 * 4. index, pointing to the current item
	 * 5. agent that knows how to increment index
	 * 6. destination object/array
	 *
	 * 2-5 are combined into an iterator, 1 and 6 are kept as is separate
	 */

	/* S cannot be virtualized because S::read depends on C */
	template<class S, class C>
	static bool read(const S& s, C& dst, lexer& in) noexcept {
		I id;
		ctype ct;
		if( ! isvalid(ct=in.value(I::start)) ) return false;
		if( ct == ctype::null ) {
			if( s.null(dst) ) return true;
			in.error(error_t::mismatch);
			return false;
		}
		char_t chr;
		in.get(chr, I::middle);
		if( ! in.skipws(chr) ) return false;
		if( chr == I::finish ) return true;
		in.back(chr);
		chr = literal::value_separator;

		do switch( chr )  {
		case I::finish: return true;
		case literal::value_separator:
			if( ! id.prolog(in) ) return false;
			if( s.read(dst, in, id++) ) continue;
			if( in.skip(I::skiplist) ) continue;
			/* no break */
		default:
			return false;
		} while( isvalid(ct=in.skip(chr, ctype::whitespace))
				&& isvalid(andmask(ct, I::middle)) );
		in.error(error_t::bad);
		return false;
	}
};

/**
 * JSON array
 */
struct array : value {
	inline array(const item* const itemlist, size_t length) noexcept
	: items(itemlist), size(length) {}

	bool read(lexer& in) const noexcept {
		return collection<>::read(*this, void_v, in);
	}
	bool write(ostream& out) const noexcept {
		return write(*this, out);
	}

	template<class A, class C>
	static inline bool write(const A& agent,
							 const C& dst, ostream& out) noexcept {
		size_t i = 0;
		while(array::dlm(i==0, out) && agent.write(dst,out,i++));
		return array::end(out);
	}
	static inline bool dlm(bool first, ostream& out) noexcept {
		return out.put(first ? literal::begin_array : literal::value_separator);
	}
	static inline bool end(ostream& out) noexcept {
		return out.put(literal::end_array);
	}

	/** write array implementation with item writing delegated to agent */
	template<class A>
	static inline bool write(const A& agent, ostream& out) noexcept {
		size_t i = 0;
		while( array::dlm(i==0, out) && agent.write(out,i++));
		return array::end(out);
	}
private:
	friend class collection<>;

	/** read array item implementation, returns false when last item read */
	inline bool read(void_t, lexer& in, size_t i) const noexcept {
		if( i < size ) {
			if( items[i]().read(in) )
				return true;
			return in.skip(false);
		} else {
			in.error(error_t::overrun);
			return false;
		}
	}

	inline bool write(ostream& out, size_t i) const noexcept {
		items[i]().write(out);
		return i+1 < size;
	}

private:
	const item* const items;
	const size_t size;
};

/**
 * JSON member - a named element in an object
 */
struct member {
private:
	template<class C> friend struct property;
	template<class C> friend struct clas;
	friend class object;

	virtual cstring name() const noexcept = 0;
	virtual bool readval(lexer&) const noexcept = 0;
	virtual bool writeval(ostream&) const noexcept = 0;

	static inline bool prolog(cstring name, ostream& out) noexcept {
		return writer<cstring>::write(name, out)
			&& out.put(literal::name_separator);
	}
	inline bool prolog(ostream& out) const noexcept {
		return prolog(name(), out);
	}
	inline bool match(const char_t* aname) const noexcept {
		return details::match(name(),aname);
	}
};

/**
 * JSON object - a collection of members
 */
struct object : value {
	object(const node* list, size_t length) : nodes(list), size(length) {}
	bool read(lexer& in) const noexcept {
		return collection<indexer>::read(*this,void_v,in);
	}
	bool write(ostream& out) const noexcept;
	static inline bool dlm(bool begin, ostream& out) noexcept {
		return out.put(begin ? literal::begin_object:literal::value_separator);
	}
	static inline bool end(ostream& out) noexcept {
		return out.put(literal::end_object);
	}
	inline bool read(void_t, lexer& i,
					 const char_t * n) const noexcept {	 return read(i, n);	}
private:
	bool read(lexer& in, const char_t * name) const noexcept;
	const node * const nodes;
	const size_t size;
};

/**
 * property - a named property of c++ class or structure
 */
template<class C>
struct property : noncopyable {
	/* property cannot be derived from value or member because of its
	 * high connection to the class instance
	 */
	typedef const property& (*node)();
	virtual cstring name() const noexcept = 0;
	virtual bool read(C& obj, lexer&) const noexcept = 0;
	virtual bool write(const C& obj, ostream&) const noexcept = 0;
	inline bool match(const char_t* aname) const noexcept {
		return details::match(name(),aname);
	}
	static inline bool constexpr null(C&) noexcept {
		return false; /* not possible to nullify */
	}
};

/**
 * clas - a c++ class or structure mapped to json object
 */
template<class C>
struct clas : noncopyable {
	typedef typename property<C>::node node;
	clas(const node * n, size_t s) noexcept : nodes(n), size(s) { }
	bool read(C& obj, lexer& in) const noexcept {
		return collection<indexer>::read(*this, obj, in);
	}
	bool write(const C& obj, ostream& out) const noexcept {
		bool r = true;
		for(size_t i = 0; i < size && r; ++i) {
			const property<C>& prop(nodes[i]());
			r = object::dlm(i==0, out) 			&&
				member::prolog(prop.name(), out)&&
				prop.write(obj, out);
		}
		return r && object::end(out);
	}
	static inline constexpr bool null(C&) noexcept {
		return config::null_is_error;
	}
protected:
	friend class collection<indexer>;
	inline bool read(C& obj, lexer& in, const char_t * name) const noexcept {
		for(size_t i = 0; i < size; ++i) {
			const property<C>& m(nodes[i]());
			if( m.match(name) ) {
				m.read(obj, in);
				return true;
			}
		}
		return false;
	}
	const node * nodes;
	const size_t size;
};

/**
 * scalar value read/write implementation based on externalized accessor X
 */
template<class X>
struct scalar : value {
	typedef typename X::type T;
	bool read(lexer& in) const noexcept {
		if( X::canlref && X::has() ) {
			return reader<T>::read(X::lref(), in);
		} else if( X::canset ) {
			T v;
			X::init(v);
			if( reader<T>::read(v, in) ) {
				X::set(v);
				return true;
			} else
				return false;
		} else {
			in.error(error_t::noobject);
			return in.skip();
		}
	}
	bool write(ostream& out) const noexcept {
		if( X::has() ) {
			if( X::canrref ) {
				return writer<T>::write(X::rref(), out);
			} else if( X::canget ) {
				return writer<T>::write(X::get(), out);
			}
		}
		return value::null(out);
	}
	bool null()  const noexcept {
		return X::null();
	}
};


/**
 * string value implementation
 */
struct string : value {
	inline string(char_t* s, size_t length) noexcept
	  : str(s), size(length) {}
	inline string(const char_t* s) noexcept
	  : str(const_cast<char_t*>(s)), size(0) {}
	bool read(lexer& in) const noexcept {
		ctype ct;
		if( ! isvalid(ct=in.value(ctype::stringnull)) )
			return in.skip();
		if( ct == ctype::null ) {
			if( null() ) return true;
			in.error(error_t::mismatch);
			return false;
		}
		return reader<char_t*>::read(str, size, in);
	}
	bool write(ostream& out) const noexcept {
		return str != nullptr ?
			writer<char_t*>::write(str, out) : value::null(out);
	}
	inline bool null() const noexcept {
		if( str != nullptr )
			*str = 0;
		return true;
	}

private:
	char_t* const str;
	const size_t size;
};

/**
 * property read/write implementation based on externalized accessor X
 */
template<class X>
struct propertyx : property<typename X::clas> {
	typedef typename X::type T;
	typedef typename X::clas C;
	bool read(C& obj, lexer& in) const noexcept {
		if( X::canlref && X::has() ) {
			return reader<T>::read(X::lref(obj), in);
		} else if( X::canset ) {
			T v;
			X::init(v);
			if( reader<T>::read(v, in) ) {
				X::set(obj, v);
				return true;
			} else
				return false;
		} else {
			in.error(error_t::noobject);
			return in.skip();
		}
	}
	bool write(const C& obj, ostream& out) const noexcept {
		if( X::canrref ) {
			return writer<T>::write(X::rref(obj), out);
		} else if( X::canget ) {
			return writer<T>::write(X::get(obj), out);
		}
		return value::null(out);
	}
};

/**
 * vector read/write implementation based on externalized accessor X
 */
template<class X>
struct vector : value {
	typedef typename X::type T;
	/** read array */
	bool read(lexer& in) const noexcept {
		if( X::canset || X::canlref )
			return collection<>::read(*this, void_v, in);
		else {
			in.error(error_t::noobject);
			return in.skip();
		}
	}
	bool write(ostream& out) const noexcept {
		if( X::canget || X::canrref )
			return array::write(*this, out);
		else
			return value::null(out);
	}

private:
	friend class array;
	friend class collection<>;
	static inline bool null(void_t) noexcept { return X::null(void_v); }
	/** read item */
	inline bool read(void_t, lexer& in, size_t i) const noexcept {
		if( X::has(i) ) {
			T tmp;
			X::init(tmp);
			if( reader<T>::read(tmp, in) ) {
				X::set(i,tmp);
				return true;
			} else
				return in.skip(false);
		} else {
			in.error(error_t::overrun);
			return false;
		}
	}
	inline bool write(ostream& out, size_t i) const noexcept {
		writer<T>::write(X::get(i), out);
		return X::has(i+1);
	}

};

/**
 * value implementation selector based on accessor type
 */
template<class X>
struct values : scalar<X> {};
//  : std::conditional<X::is_vector, vector<X>, scalar<X>>::type { };

/**
 * object as a value read/write implementation based on externalized accessor X
 */
template<class X, const clas<typename X::clas>& (*S)() noexcept>
struct objectval : value {
	bool read(lexer& in) const noexcept {
		if( X::canlref && X::has() ) {
			return S().read(X::lref(), in);
		} else {
			in.error(error_t::noobject);
			return in.skip();
		}
	}
	bool write(ostream& out) const noexcept {
		if( X::canrref && X::has() ) {
			return S().write(X::rref(), out);
		} else {
			return null(out);
		}
	}
};

/**
 * array of objects read/write implementation based on externalized accessor X
 */
template<class X, const clas<typename X::type>& (*S)() noexcept>
struct objectlist : value {

	bool read(lexer& in) const noexcept {
		if( X::canset ) {
			//return array::read(*this,in);
			return collection<>::read(*this, void_v, in);
		} else {
			in.error(error_t::noobject);
			return in.skip();
		}
	}
	bool write(ostream& out) const noexcept {
		if( X::canget )
			return array::write(*this,out);
		else
			return value::null(out);
	}
	static inline bool null() noexcept {
		return X::null();
	}
private:
	friend class array;
	friend class collection<>;

	inline bool read(const void_t&, lexer& in, size_t i) const noexcept {
		if( X::has(i) ) {
			if( S().read(X::lref(i), in) )
				return true;
			else
				return in.skip(false);
		} else {
			in.error(error_t::overrun);
			return false;
		}
	}
	template<typename T>
	static inline constexpr bool null(T v) noexcept {
		return X::null(v);
	}

	inline bool write(ostream& out, size_t i) const noexcept {
		return S().write(X::get(i), out) && X::has(i+1);
	}
};

template<class X, const clas<typename X::type>& (*S)() noexcept>
struct objects
  : std::conditional<X::is_vector,
		objectlist<X,S>, objectval<X,S>>::type {
};

}

/**
 * scalar class property
 */
template<class C, details::name id, typename T, T C::*V>
const details::property<C> & P() noexcept {
	static const struct local : details::propertyx<accessor::field<C,T,V>> {
		cstring name() const noexcept { return id(); }
	} l;
	return l;
}

/**
 * scalar class property via getter/setter wrapped in accessor
 */
template<class C, details::name id, class X>
const details::property<C> & P() noexcept {
	static const struct local : details::propertyx<X> {
		cstring name() const noexcept { return id(); }
	} l;
	return l;
}

/**
 * string class property
 */
template<class C, details::name id, size_t N, char_t (C::*M)[N]>
const details::property<C> & P() noexcept {
	static const struct local : details::property<C> {
		cstring name() const noexcept { return id(); }
		bool read(C& obj, details::lexer& in) const noexcept {
			return details::reader<char_t*>::read(obj.*M, N, in);
		}
		bool write(const C& obj, details::ostream& out) const noexcept {
			return details::writer<const char_t*>::write(obj.*M, out);
		}
	} l;
	return l;
}

/**
 * vector class property (T[N])
 */
template<class C, details::name id, typename T, size_t N, T (C::*M)[N]>
const details::property<C>& P() {
	static const struct local : details::property<C> {
		cstring name() const noexcept { return id(); }
		bool read(C& obj, details::lexer& in) const noexcept {
			return details::collection<>::read(*this, obj, in);
		}
		bool write(const C& obj, details::ostream& out) const noexcept {
			/* delegate write to array */
			return details::array::write(*this, obj, out);
		}
		/** read item */
		inline bool read(C& obj, details::lexer& in, size_t i) const noexcept {
			return
				( details::reader<T>::read((obj.*M)[i], in) || in.skip(false) )?
				(i < N-1) : false;
		}
		/** write item item */
		inline bool write(const C& obj, details::ostream& out,
				size_t i) const noexcept {
			details::writer<T>::write((obj.*M)[i], out);
			return i < N-1;
		}
	} l;
	return l;
}

/**
 * nested in C object property of type T with structure S
 */
template<class C,details::name id,class T,T C::*V,const details::clas<T>& S()>
const details::property<C> & P() {
	static const struct local : details::property<C> {
		cstring name() const noexcept { return id(); }
		bool read(C& obj, details::lexer& in) const noexcept {
			return S().read(obj.*V, in);
		}
		bool write(const C& obj, details::ostream& out) const noexcept {
			return S().write(obj.*V, out);
		}
	} l;
	return l;
}

/**
 * nested in C array of objects of type T with structure S
 */
template<class C, details::name id, class T,
	size_t N, T (C::*V)[N], const details::clas<T>& S()>
const details::property<C> & P() {
	static const struct local : details::property<C> {
		cstring name() const noexcept { return id(); }
		bool read(C& obj, details::lexer& in) const noexcept {
			return details::collection<>::read(*this, obj, in);
		}
		bool write(const C& obj, details::ostream& out) const noexcept {
			return details::array::write(*this, obj, out);
		}
		/** read item */
		inline bool read(C& obj, details::lexer& in, size_t i) const noexcept {
			S().read((obj.*V)[i], in);
			return i < N-1;
		}
		/** write item item */
		inline bool write(const C& obj, details::ostream& out,
				size_t i) const noexcept {
			S().write((obj.*V)[i], out);
			return i < N-1;
		}
	} l;
	return l;
}

/**
 * JSON object associated with a C++ class
 */
template<class C, typename details::property<C>::node ... L>
const details::clas<C>& O() noexcept {
	static constexpr typename details::property<C>::node list[] { L ... } ;
	static constexpr auto size = sizeof...(L);
	static const details::clas<C> l(list,size);
	return l;
}

/**
 * JSON object
 */
template<details::node ... L>
const details::value& V() noexcept {
	static constexpr details::node list[] { L ... };
	static constexpr unsigned size = sizeof...(L);
	static const details::object l(list, size);
	return l;
}


/**
 * JSON array (heterogeneous list)
 */
template<details::item ... L>
const details::value& V() noexcept {
	static constexpr details::item items[] { L ... };
	static constexpr auto size = sizeof...(L);
	static details::array l(items, size);
	return l;
}

/**
 * JSON string bound to an array char_t[N]
 */
template<size_t N, char_t (&A)[N]>
const details::value& V() noexcept {
	static const details::string l(A,N);
	return l;
}


/**
 * JSON string bound to an array char_t[N] via function F
 */
template<size_t N, char_t* (*F)() noexcept>
const details::value& V() noexcept {
	static const details::string l(F(),N);
	return l;
}

/**
 * not parseable string bound to a zero-teminated string via function F
 */
template<const char_t* (*F)() noexcept>
const details::value& V() noexcept {
	static const details::string l(F());
	return l;
}

/**
 * a single scalar value or a vector of scalars of unspecified length
 * accessed via accessor class X
 */
template<class X>
const details::value& V() noexcept {
	static const details::values<X> l;
	return l;
}

/**
 * a object or a vector of object unspecified length
 * accessed via accessor class X and structured with S.
 * @param
 */
template<class X, const details::clas<typename X::clas>& (*S)() noexcept>
const details::value& V() noexcept {
	static const details::objects<X,S> l;
	return l;
}

/**
 * value - plain variable via pointer
 */
template<typename T, T* P>
const details::value& V() noexcept {
	static const details::scalar<accessor::pointer<T,P>> l;
	return l;
}


/**
 * value - plain variable by function returning reference
 */
template<typename T, T& (*F)() noexcept>
const details::value& V() noexcept {
	static const details::scalar<accessor::reference<T,F>> l;
	return l;
}

/**
 * value - plain variable by function returning pointer
 */
template<typename T, T* (*F)() noexcept>
const details::value& V() noexcept {
	static const details::scalar<accessor::function<T,F>> l;
	return l;
}

/**
 * value - plain variable by pair getter/setter functions
 */
template<typename T, T (*G)() noexcept, void (*S)(T) noexcept>
const details::value& V() noexcept {
	static const details::scalar<accessor::functions<T,G,S>> l;
	return l;
}


/**
 * value - a vector of T accessible via function returning pointer to item
 */
template<typename T, size_t N, T (&A)[N]>
const details::value& V() noexcept {
	static const details::vector<accessor::array<T,N,A>> l;
	return l;
}

/**
 * value - a vector of T accessible via function returning pointer to item
 */
template<typename T, T* (*F)(size_t) noexcept>
const details::value& V() noexcept {
	static const details::vector<accessor::vector<T,F>> l;
	return l;
}

/**
 * JSON member (generic)
 */
template<details::name id, details::item I>
const details::member& M() noexcept {
	static const struct local : details::member {
		cstring name() const noexcept { return id(); }
		bool readval(details::lexer& in) const noexcept {
			return I().read(in);
		}
		bool writeval(details::ostream& out) const noexcept {
			return I().write(out);
		}
	} l;
	return l;
}

/**
 * JSON string member
 */
template<details::name id, size_t N, char_t* (*F)() noexcept>
const details::member& M() noexcept {
	static const struct local : details::member, details::string {
		inline local() noexcept : details::string(F(),N) {}
		cstring name() const noexcept { return id(); }
		bool readval(details::lexer& in) const noexcept { return read(in); }
		bool writeval(details::ostream& out) const noexcept { return write(out); }
	} l;
	return l;
}

/**
 * not parseable string bound to a zero-teminated string via function F
 */
template<details::name id, const char_t* (*F)() noexcept>
const details::member& M() noexcept {
	static const struct local : details::member, details::string {
		inline local() noexcept : details::string(F()) {}
		cstring name() const noexcept { return id(); }
		bool readval(details::lexer& in) const noexcept {
			return read(in);
		}
		bool writeval(details::ostream& out) const noexcept {
			return write(out);
		}
	} l;
	return l;
}


/**
 * member - by accessor
 */
template<details::name id, class X>
const details::member& M() noexcept {
	static const struct local : details::member, details::values<X> {
		cstring name() const noexcept { return id(); }
		bool readval(details::lexer& in) const noexcept {
			return details::values<X>::read(in);
		}
		bool writeval(details::ostream& out) const noexcept {
			return details::values<X>::write(out);
		}
	} l;
	return l;
}

/**
 * member - plain variable by function returning reference
 */
template<details::name id, typename T, T& (*F)() noexcept>
const details::member& M() noexcept {
	static const struct local : details::member, details::values<accessor::reference<T,F>> {
		cstring name() const noexcept { return id(); }
		bool readval(details::lexer& in) const noexcept {
			return details::values<accessor::reference<T,F>>::read(in);
		}
		bool writeval(details::ostream& out) const noexcept {
			return details::values<accessor::reference<T,F>>::write(out);
		}
	} l;
	return l;
}

/**
 * member - plain variable by function returning pointer
 */
template<details::name id, typename T, T* P>
const details::member& M() noexcept {
	static const struct local : details::member,
		details::scalar<accessor::pointer<T,P>> {
		cstring name() const noexcept { return id(); }
		bool readval(details::lexer& in) const noexcept {
			return details::scalar<accessor::pointer<T,P>>::read(in);
		}
		bool writeval(details::ostream& out) const noexcept {
			return details::scalar<accessor::pointer<T,P>>::write(out);
		}
	} l;
	return l;
}


/**
 * member - plain variable by function returning pointer
 */
template<details::name id, typename T, T* (*F)() noexcept>
const details::member& M() noexcept {
	static const struct local : details::member,
		details::scalar<accessor::function<T,F>> {
		cstring name() const noexcept { return id(); }
		bool readval(details::lexer& in) const noexcept {
			return details::scalar<accessor::function<T,F>>::read(in);
		}
		bool writeval(details::ostream& out) const noexcept {
			return details::scalar<accessor::function<T,F>>::write(out);
		}
	} l;
	return l;
}

namespace details {
/****************************************************************************
 *						Interface Implementations      						*
 ****************************************************************************/

/** merger of istream and ostream 											*/
class iostream : public ostream, public istream {
};

/**
 * AN abstract i/o stream wrapper to an external array of char_t
 */
class abuffer : public iostream {
public:
	abuffer() noexcept : iostream(), pos(0), ptr(nullptr) {  }
	abuffer(char_t* data) noexcept : pos(0), ptr(data) { }
	char_t* begin() const noexcept { return ptr; }
	virtual size_t size() const noexcept = 0;
	inline size_t count() const noexcept { return pos; }
	bool get(char_t& val) noexcept {
		if( size() == 0 ) { /* array expected to contain zero delimited string*/
			val = ptr[pos];
			if( val == 0 ) {
				val = iostate::eos_c;
				error(error_t::eof);
				return false;
			}
			++pos;
			return true;
		}
		if( pos >= size() ) {
			val = iostate::eos_c;
			error(error_t::eof);
			return false;
		}
		++pos;
		val = ptr[pos++];
		return true;
	}
	bool put(char_t val) noexcept {
		if( pos >= size() ) {
			error(error_t::eof);
			return false;
		}
		ptr[pos++] = val;
		return true;
	}
	inline void restart() noexcept {
		clear();
		pos = 0;
	}
protected:
	void  set(char_t * data) noexcept {
		if( ptr != data )
			restart();
		ptr = data;
	}
private:
	size_t 	pos;
	char_t *ptr;
};

/**
 * An obuffer wrapper to an external array of char_type
 * given by pointer and size
 */
class buffer : public abuffer {
public:
	buffer() noexcept : abuffer(nullptr), msize(0) { }
	buffer(const char_t *data) noexcept
		: abuffer(const_cast<char_t *>(data)), msize(0) { }
	buffer(char_t *data, size_t n) noexcept
		: abuffer(data), msize(n) { }
	template<size_t N>
	buffer(char_t (&data)[N]) noexcept : abuffer(data), msize(N) {}
	template<size_t N>
	buffer(const char_t (&data)[N]) noexcept : abuffer(data), msize(0) {}
	void set(char_t *data, size_t n) noexcept {
		abuffer::set(data);
		msize = n;
	}
	void set(const char_t *data) noexcept {
		abuffer::set(const_cast<char_t *>(data));
		msize = 0;
	}
	size_t size() const noexcept { return msize; }
private:
	size_t msize;
};

}} /* namespace cojson */
#endif /* COJSON_HPP_ */
