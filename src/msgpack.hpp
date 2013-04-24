/**
 * @file msgpack.hpp
 * @author  Arturo Blas Jiménez <arturoblas@gmail.com>
 * @version 0.1
 *
 * @section LICENSE
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 *
 *  This header file provides a C++ implementation of the MessagePack
 *  binary serialization protocol <http://http://msgpack.org/> relying on
 *  an extensive usage of STL containers and stream classes.
 *
 */

#ifndef MSGPACK_HPP_
#define MSGPACK_HPP_

#include <stdint.h>

#include <vector>
#include <list>
#include <deque>
#include <queue>
#include <set>
#include <map>
#include <stack>
#include <bitset>
#include <cmath>
#include <typeinfo>
#include <locale>

namespace packing
{
	
	/**
	 * This enum type defines all the possible output types
	 * for the {@see #Object} instances generated by
	 * the {@see #unpacker} class
	 */
	enum object_type
	{
		NIL, 		//<! nil
		BOOLEAN, 	//<! boolean [true, false]
		CHAR, 		//<! int8 [positive, negative fixnum]
		SHORT, 		//<! int16
		INTEGER, 	//<! int32
		LONG, 		//<! int64
		UCHAR, 		//<! uint8
		USHORT, 	//<! uint16
		UINTEGER, 	//<! uint32
		ULONG, 		//<! uint64
		FLOAT, 		//<! float
		DOUBLE, 	//<! double
		RAW, 		//<! Raw bytes [fix raw, raw 16, raw 32]
		ARRAY, 		//<! Array [fix array, array 16, array 32]
		MAP, 		//<! Map [fix map, map 16, map 32]
		
	}; // object_type
	
	// Forward declarations
	class packer;
	class unpacker;
	
	/**
	 * parcel defines an interface that should be implemented
	 * by all classes that can be serialized and deserialized
	 * to and from a stream, given the protocol implemented
	 * by the packer and unpacker classes.
	 */
	class parcel {
	public:
		
		/**
		 * Virtual method that packs the current object
		 * using the provided packer object.
		 */
		virtual void pack(packer&) const = 0;
		
		/**
		 * Virtual method that unpacks the current object
		 * data from the provided unpacker object.
		 */
		virtual void unpack(unpacker&) = 0;
		
		virtual ~parcel() {}
		
	}; // parcel
	
	/**
	 * packer class. Provides the functionality to serialize data
	 * into a stream using the a simple protocol and also allows to implement
	 * new protocols by extending the current one.
	 */
	class packer {
	public:
		
		/**
		 * Constructor
		 * @param out Packer output stream where the binary data will be put
		 */
		explicit packer(std::ostream& out) : out_(out) {}
		
		/**
		 * Packs a parcel object using the current packer
		 * object.
		 * @param p reference to a parcel object.
		 */
		packer& pack(const parcel& p)
		{
			p.pack(*this);
			return *this;
		}
		
		/**
		 * Packs using a pointer as the data source. If the pointer is null
		 * a null object will be serialized into the stream, if not,
		 * the contents of the pointer will be serialized.
		 * @param item Pointer to the data to be serialized.
		 */
		template<class T>
		packer& pack(const T* item)
		{
			return (item == 0) ? packNull() : pack(*item);
		}
		
		/**
		 * Packs using a C string as a source of data. If the pointer is null
		 * a null object will be serialized into the stream, if not,
		 * the contents of the array will be serialized using the lenght of
		 * the string provided by strlen(). Note that this is not suitable for
		 * serializing raw regions of memory, in order to do so,
		 * {@see #Packer::pack(const char*, std::size) } should be used instead.
		 * @param item Pointer to the C string to be serialized.
		 */
		virtual packer& pack(const char* item)
		{
			return (item == 0) ? packNull() : pack(item, strlen(item));
		}
		
		/**
		 * Packs using a wide char string as a source of data. If the pointer is null
		 * a null object will be serialized into the stream, if not,
		 * the contents of the array will be serialized using the lenght of
		 * the string provided by wcslen().
		 * @param item Pointer to the string to be serialized.
		 */
		virtual packer& pack(const wchar_t* item)
		{
			return (item == 0) ? packNull() : pack((char*) item, wcslen(item) * sizeof(wchar_t));
		}
		
		/**
		 * Packs the null object into the stream
		 */
		virtual packer& packNull()
		{
			return writeType(NIL);
		}
		
		/**
		 * Pack a boolean value
		 * @param item Value to be serialized
		 */
		virtual packer& pack(bool item)
		{
			return writeType(BOOLEAN).write(item);
		}
		
		/**
		 * Pack an integer value
		 * @param item Value to be serialized
		 */
		virtual packer& pack(int item)
		{
			return writeType(INTEGER).write(item);
		}
		
		/**
		 * Pack a long integer value
		 * @param item Value to be serialized
		 */
		virtual packer& pack(long item)
		{
			return writeType(LONG).write(item);
		}
		
		/**
		 * Pack a single precision floating point value
		 * @param item Value to be serialized
		 */
		virtual packer& pack(float item)
		{
			return writeType(FLOAT).write(item);
		}
		
		/**
		 * Pack a double precision floating point value
		 * @param item Value to be serialized
		 */
		virtual packer& pack(double item)
		{
			return writeType(DOUBLE).write(item);
		}
		
		/**
		 * Pack a basic_string into the current stream.
		 * @param value reference to the string that will be serialized.
		 */
		template<class T>
		inline packer& pack(const std::basic_string<T>& value)
		{
			return pack(value.c_str());
		}
		
		/**
		 * Pack a raw memory region with the provided length in bytes into
		 * the output stream.
		 * @param data Pointer to the first byte of the memory region.
		 * @param length Size in bytes of the data region.
		 */
		virtual packer& pack(const char* data, std::size_t length)
		{
			return writeType(RAW).write(length).write(data, length);
		}
		
		/**
		 * This allows to serialize a range of items within a container
		 * object by specifying the first and the last position to be
		 * written.
		 * @param first Iterator pointing to the first position.
		 * @param last Iterator pointing to the position next to the
		 * last one to be written.
		 */
		template<class IteratorT> inline
		packer& pack(IteratorT first, IteratorT last)
		{
			typename std::iterator_traits<IteratorT>::value_type type;
			
			std::size_t length = std::distance(first, last);
			this->init_container(length, type);
			
			// perform function for each element
			for (; first != last; ++first)
				pack(*first);
			
			return *this;
		}
		
		/**
		 * Pack a std::pair object
		 * @param item reference to the pair
		 */
		template<class keyT, class valT> inline
		packer& pack(const std::pair<const keyT, valT>& item)
		{
			return pack(item.first).pack(item.second);
		}
		
		/**
		 * Pack a std::vector object
		 * @param item reference to the vector
		 */
		template<typename T> inline
		packer& pack(const std::vector<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::deque object
		 * @param item reference to the deque
		 */
		template<typename T> inline
		packer& pack(const std::deque<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::list object
		 * @param item reference to the list
		 */
		template<typename T> inline
		packer& pack(const std::list<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::stack object
		 * @param item reference to the stack
		 */
		template<typename T> inline
		packer& pack(const std::stack<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::queue object
		 * @param item reference to the queue
		 */
		template<typename T> inline
		packer& pack(const std::queue<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::priority_queue object
		 * @param item reference to the priority_queue
		 */
		template<typename T> inline
		packer& pack(const std::priority_queue<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::set object
		 * @param item reference to the set
		 */
		template<typename T> inline
		packer& pack(const std::set<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::multiset object
		 * @param item reference to the multiset
		 */
		template<typename T> inline
		packer& pack(const std::multiset<T>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::map object
		 * @param item reference to the map
		 */
		template<typename T, typename U> inline
		packer& pack(const std::map<T, U>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::multimap object
		 * @param item reference to the multimap
		 */
		template<typename T, typename U> inline
		packer& pack(const std::multimap<T, U>& item)
		{
			return pack(item.begin(), item.end());
		}
		
		/**
		 * Pack a std::bitset object. Each bit is packed as a char
		 * of a raw in the output stream
		 * @param item reference to the bitset
		 */
		template<size_t N> inline
		packer& pack(const std::bitset<N>& item)
		{
			static size_t size = N;
			char p[(int) std::ceil(float(size) / float(8))] = { 0 };
			for (int i = 0; i < 32; i++) {
				p[i / 8] |= (item[i] == '1') << 7 - i % 8;
			}
			return pack(&p[0], size);
		}
		
		virtual ~packer() {}
		
	protected:
		
		/**
		 * Write data onto the stream
		 */
		template<typename T> inline packer& write(T data) {
			out_.write((char*) &data, sizeof(T));
			return *this;
		}
		
		/**
		 * Write a bunch of data onto the stream
		 */
		inline packer& write(const char* data, std::size_t length) {
			out_.write(data, length);
			return *this;
		}
		
		/**
		 * This packs the header of an array for a given length
		 */
		virtual packer& packArrayHeader(std::size_t& length) {
			return writeType(ARRAY).write(length);
		}
		
		/**
		 * This packs the header of a map for a given length
		 */
		virtual packer& packMapHeader(std::size_t& length) {
			return writeType(MAP).write(length);
		}
		
	private:
		
		template<class T> inline
		void init_container(std::size_t& length, T&) {
			packArrayHeader(length);
		}
		
		template<class key, class val> inline
		void init_container(std::size_t& length, std::pair<const val, key>&) {
			packMapHeader(length);
		}
		
		// Pack the object type with char length
		inline packer& writeType(object_type t) {
			return write<char>(t);
		}
		
		std::ostream& out_; //!< The output stream where the data is packed in
	};
	
	/**
	 * Exception likely to be thrown during the
	 * data deserialization.
	 */
	typedef std::ios_base::failure unpack_exception;
	
	namespace detail
	{
		
		/**
		 * This defines the primary relationship
		 * between an object_type and the data
		 * type that is actually handled by it
		 */
		template<object_type t>
		struct type_traits {
		};
		
		/**
		 * This defines the possible casts
		 * from a given type T to an object type.
		 */
		template<typename T>
		struct type_cast {
		};
		
		// Forward definition
		template<object_type type>
		class ObjectImpl;
		
	} // namespace detail
	
	/**
	 * Object class. Represents a piece of deserialized
	 * data, and provides access to the data itself.
	 */
	class Object {
	public:
		
		/**
		 * Destructor
		 */
		virtual ~Object() {
		}
		
		/**
		 * Retrieve the object type for the
		 * current Object.
		 */
		object_type getType() const {
			return type_;
		}
		
		/**
		 * Get current object value as type T. Note that the type_cast should
		 * be specified for this to work.
		 */
		template<typename T>
		T getValue() const throw (std::bad_cast) {
			return (T) getAs<detail::type_cast<T>::id>();
		}
		
		/**
		 * Return the current Object data as the specified
		 * type provided as a template parameter.
		 * In case the provided type is not current Object's type
		 * a std::bad_cast exception is thrown, otherwise, the
		 * method returns a reference to the inner data.
		 */
		template<object_type type>
		typename detail::type_traits<type>::type getValue() const
		throw (std::bad_cast) {
			return getValue<typename detail::type_traits<type>::type>();
		}
		
		/**
		 * Return the current object as the specific implementation for the given
		 * object_type.
		 */
		template<object_type type>
		const typename detail::ObjectImpl<type>& getAs() const throw (std::bad_cast) {
			if (type != type_)
				throw std::bad_cast();
			
			return dynamic_cast<const typename detail::ObjectImpl<type>&>(*this);
		}
		
		/**
		 * Return the current object as the specific implementation for the given
		 * type.
		 */
		template<typename T>
		const typename detail::ObjectImpl<detail::type_cast<T>::id>& getAs() const
		throw (std::bad_cast) {
			return getAs<detail::type_cast<T>::id>();
		}
		
		/**
		 * Returns true if this object represents
		 * a nil instance.
		 */
		virtual bool isNil() const {
			return false;
		}
		
	protected:
		
		/**
		 * Protected constructor. This is a virtual class.
		 * @param type Type handled by the current object instance.
		 */
		explicit Object(object_type type) :
		type_(type) {
		}
		
		Object() : type_(NIL) {
		}
		
	private:
		const object_type type_; //!< Type of the current object
		
	}; // Object
	
	namespace detail
	{
		
		/**
		 * Class ObjectImpl. Internal implementation for the specific Object types
		 * that handle fixed length types, except for nil.
		 */
		template<object_type type>
		class ObjectImpl: public Object {
		public:
			typedef typename detail::type_traits<type>::type T; //<! My type
			
			/**
			 * Constructor.
			 * @param value Value contained by the Object. There's a implicit
			 * copy performed here.
			 */
			explicit ObjectImpl(const T& value) :
			Object(type), value_(value) {
			}
			
			/**
			 * Cast operator
			 */
			operator const T&() const {
				return value_;
			}
			
		private:
			
			T value_; //< Instance of the data handled by the Object
			
		}; // ObjectImpl
		
		/**
		 * Nil class. Represents a nil object.
		 */
		template<>
		class ObjectImpl<NIL> : public Object {
		public:
			
			ObjectImpl() : Object(NIL) {} //<! Constructor
			
			bool isNil() const {
				// I'm the nil guy!
				return true;
			}
		};
		
		/**
		 * Declaration of the types correspondences with
		 * their specific data types
		 */
		
#define TYPE_CAST(__ID__, __TYPE__) \
template<> \
struct type_cast<__TYPE__> \
{ \
static const object_type id = __ID__;\
};
		
#define TYPE_TRAITS(__ID__, __TYPE__) \
template<> \
struct type_traits<__ID__> \
{ \
typedef __TYPE__ type; \
}; \
TYPE_CAST(__ID__, __TYPE__)
		
		TYPE_TRAITS(BOOLEAN, bool)
		TYPE_TRAITS(CHAR, char)
		TYPE_TRAITS(SHORT, int16_t)
		TYPE_TRAITS(INTEGER, int32_t)
		TYPE_TRAITS(LONG, int64_t)
		TYPE_TRAITS(UCHAR, unsigned char)
		TYPE_TRAITS(USHORT, uint16_t)
		TYPE_TRAITS(UINTEGER, uint32_t)
		TYPE_TRAITS(ULONG, uint64_t)
		TYPE_TRAITS(FLOAT, float)
		TYPE_TRAITS(DOUBLE, double)
		
		TYPE_TRAITS(RAW, unsigned char*)
		TYPE_CAST(RAW, std::vector<unsigned char>)
		TYPE_CAST(RAW, std::string)
		// Type cast is implemented for raw type
		TYPE_CAST(RAW, std::wstring)
		
		TYPE_TRAITS(ARRAY, std::list<Object*>)
		
		typedef std::multimap<Object*, Object*> map_type;
		TYPE_TRAITS(MAP, detail::map_type)
		
		/**
		 * ObjectImpl class specilization for the ARRAY type.
		 */
		template<>
		class ObjectImpl<ARRAY> : public Object {
		public:
			typedef type_traits<ARRAY>::type array_t; //!< Own array data type
			
			/**
			 * Destructor. Removes and deletes all the Objects
			 * contained in the array.
			 */
			virtual ~ObjectImpl() {
				array_t::iterator it = value_.begin();
				;
				while (it != value_.end()) {
					delete *it;
					*it = 0;
					it++;
				}
				value_.clear();
			}
			
			ObjectImpl() : Object(ARRAY) {} //!< Constructor
			
			/**
			 * Cast operator
			 */
			operator const array_t&() const {
				return value_;
			}
			
			/**
			 * Add a new object to the array, which
			 * will be owned and deleted by it on destruction.
			 */
			void add(Object* o) {
				value_.push_back(o);
			}
			
		private:
			array_t value_; //!< Instance of the array handled by the Object
		};
		
		/**
		 * ObjectImpl class specilization for the MAP type.
		 */
		template<>
		class ObjectImpl<MAP> : public Object {
		public:
			typedef type_traits<MAP>::type map_t; //!< Own map data type
			
			/**
			 * Destructor. Removes and deletes all the Object keys
			 * and values contained in the map.
			 */
			virtual ~ObjectImpl() {
				map_t::iterator it = value_.begin();
				;
				while (it != value_.end()) {
					delete it->first;
					delete it->second;
					it++;
				}
				value_.clear();
			}
			
			ObjectImpl() :
			Object(MAP) {
			} //!< Constructor
			
			/**
			 * Cast operator
			 */
			operator const map_t&() const {
				return value_;
			}
			
			/**
			 * Insert a new pair key-value into the map, which
			 * will be owned and deleted by it on destruction.
			 */
			void insert(Object* key, Object* val) {
				value_.insert(map_t::value_type(key, val));
			}
			
		private:
			map_t value_; //!< Instance of the map handled by the Object
		};
		
		/**
		 * ObjectImpl class specilization for the RAW type.
		 */
		template<>
		class ObjectImpl<RAW> : public Object {
		public:
			typedef type_traits<RAW>::type raw_t; //!< Own raw data type
			
			/**
			 * Constructor
			 * @value Reference to the data region handled
			 * by the Object, which ownership is now from
			 * the current Object which will destroy it on
			 * deletion.
			 */
			ObjectImpl(raw_t value, std::size_t size) :
			Object(RAW), value_(value), size_(size) {
			}
			
			/**
			 * Destructor
			 */
			virtual ~ObjectImpl() {
				delete[] value_;
			}
			
			/**
			 * Cast operator
			 */
			operator const raw_t&() const {
				return value_;
			}
			
			operator std::vector<unsigned char>() const {
				std::vector<unsigned char> v;
				std::copy(value_, value_ + size_, std::back_inserter(v));
				return v;
			}
			
			operator std::wstring() const {
				std::wstring str;
				std::copy((wchar_t*) value_, (wchar_t*) (value_ + size_),
						  std::back_inserter(str));
				return str;
			}
			
			operator std::string() const {
				std::string str((char*) value_, size_);
				return str;
			}
			
		protected:
			
			ObjectImpl() {
			}
			ObjectImpl(const ObjectImpl& other) {
			}
			void operator=(const ObjectImpl& other) {
			}
			
		private:
			raw_t value_; //!< Pointer to the instance of the buffer handled by the Object
			std::size_t size_;
		};
		
		typedef detail::ObjectImpl<NIL> Nil;
		typedef detail::ObjectImpl<BOOLEAN> Bool;
		typedef detail::ObjectImpl<CHAR> Char;
		typedef detail::ObjectImpl<SHORT> Short;
		typedef detail::ObjectImpl<INTEGER> Int;
		typedef detail::ObjectImpl<LONG> Long;
		typedef detail::ObjectImpl<UCHAR> UnsignedChar;
		typedef detail::ObjectImpl<USHORT> UnsignedShort;
		typedef detail::ObjectImpl<UINTEGER> UnsignedInt;
		typedef detail::ObjectImpl<ULONG> UnsignedLong;
		typedef detail::ObjectImpl<FLOAT> Float;
		typedef detail::ObjectImpl<DOUBLE> Double;
		typedef detail::ObjectImpl<RAW> Raw;
		typedef detail::ObjectImpl<ARRAY> Array;
		typedef detail::ObjectImpl<MAP> Map;
		
		//! Utility struct that allows to remove the pointer part of a given type
		template<typename T>
		struct remove_pointer {
			typedef T type;
		};
		template<typename T>
		struct remove_pointer<T*> {
			typedef typename remove_pointer<T>::type type;
		};
		
	} // namespace detail
	
	/**
	 * unpacker class. Provides the functionality to deserialize data
	 * from a stream using the a the protocol implemented by the packer
	 * class and also allows to implement new protocols by extending it.
	 */
	class unpacker
	{
	public:
		
		/**
		 * Constructor
		 * @param in Unpacker input stream from where the binary data is taken
		 */
		unpacker(std::istream& in) : in_(in)
		{
		}
		
		/**
		 * Unpacks a parcel object with the current
		 * unpacker.
		 * @param p parcel which data will be loaded from the stream.
		 */
		unpacker& unpack(parcel& p)
		{
			p.unpack(*this);
			return *this;
		}
		
		/**
		 * Constructs an Object from the available data in the input
		 * stream. The caller is responsible for deleting the returned
		 * instance.
		 * This method may throw an {@see #unpack_exception} in case the
		 * buffer runs out of data while trying to deserialize.
		 */
		virtual Object* unpack() throw (packing::unpack_exception)
		{
			using namespace detail;
			
			char c;
			read<char>(c);
			object_type type = (object_type) c;
			
			type_traits<BOOLEAN>::type boolVal;
			type_traits<FLOAT>::type fVal;
			type_traits<DOUBLE>::type dVal;
			type_traits<CHAR>::type cVal;
			type_traits<SHORT>::type sVal;
			type_traits<INTEGER>::type iVal;
			type_traits<LONG>::type lVal;
			type_traits<UCHAR>::type ucVal;
			type_traits<USHORT>::type usVal;
			type_traits<UINTEGER>::type uiVal;
			type_traits<ULONG>::type ulVal;
			
			std::size_t length;
			
			switch (type) {
				case NIL:
					return new Nil();
				case BOOLEAN:
					read(boolVal);
					return new Bool(boolVal);
				case FLOAT:
					read(fVal);
					return new Float(fVal);
				case DOUBLE:
					read(dVal);
					return new Double(dVal);
				case CHAR:
					read(cVal);
					return new Char(cVal);
				case SHORT:
					read(sVal);
					return new Short(sVal);
				case INTEGER:
					read(iVal);
					return new Int(iVal);
				case LONG:
					read(lVal);
					return new Long(lVal);
				case UCHAR:
					read(ucVal);
					return new UnsignedChar(ucVal);
				case USHORT:
					read(usVal);
					return new UnsignedShort(usVal);
				case UINTEGER:
					read(uiVal);
					return new UnsignedInt(uiVal);
				case ULONG:
					read(ulVal);
					return new UnsignedLong(ulVal);
				case RAW:
					read(length);
					return unpackRaw(length);
				case ARRAY:
					read(length);
					return unpackArray(length);
				case MAP:
					read(length);
					return unpackMap(length);
				default:
					throw unpack_exception("Unexpected data type");
			}
		}
		
		virtual ~unpacker() {}
		
	protected:
		
		/**
		 * Read data from the stream.
		 * @param ret Where the readed data is copied
		 */
		template<typename T> inline unpacker& read(T& ret) throw (unpack_exception)
		{
			if (in_.read((char*) &ret, sizeof(T)).eof())
				throw unpack_exception("Reached end of stream while reading");
			
			return *this;
		}
		
		/**
		 * Creates an array of the given size from the input stream
		 */
		inline packing::detail::Array* unpackArray(int size)
		{
			if (size < 0)
				return 0;
			
			packing::detail::Array* ret = new packing::detail::Array();
			for (int i = 0; i < size; ++i)
			{
				ret->add(unpack());
			}
			return ret;
		}
		
		
		/**
		 * Creates an map of the given size from the input stream
		 */
		inline detail::Map* unpackMap(int size)
		{
			if (size < 0)
				return 0;
			
			detail::Map* ret = new detail::Map();
			
			for (int i = 0; i < size; ++i) {
				Object* key = unpack();
				Object* val = unpack();
				ret->insert(key, val);
			}
			return ret;
		}
		
		/**
		 * Reads a raw region of memory from the input stream
		 */
		inline detail::Raw* unpackRaw(int size)
		{
			if (size < 0)
				return 0;
			
			typedef detail::type_traits<RAW>::type data_type;
			
			data_type data = new detail::remove_pointer<data_type>::type[size];
			for (int i = 0; i < size; i++)
				read(data[i]);
			
			return new detail::Raw(data, size);
		}
		
	private:
		
		std::istream& in_; //!< The stream we are unpacking the data from
	};
	
	template<class unpacker_t>
	class unpacker_type
	{
	public:
		typedef unpacker_t type;
		
		unpacker_type() : unpacker_(0) {}
		
		~unpacker_type()
		{
			if(unpacker_)
			{
				delete unpacker_;
				unpacker_ = 0;
			}
		}
		
		type& getUnpacker(std::istream& is)
		{
			if(unpacker_)
			{
				delete unpacker_;
			}
			unpacker_ = new type(is);
			return *unpacker_;
		}
	private:
		type* unpacker_;
	};
	
	template<class packer_t>
	class packer_type
	{
	public:
		typedef packer_t type;
		
		packer_type() : packer_(0) {}
		
		~packer_type()
		{
			if(packer_)
			{
				delete packer_;
				packer_ = 0;
			}
		}
		
		type& getPacker(std::ostream& is)
		{
			if(packer_)
			{
				delete packer_;
			}
			packer_ = new type(is);
			return *packer_;
		}
		
	private:
		type* packer_;
	};
	
	/**
	 * Define the pack and unpack helpers for the current namespace
	 * This allows doing things like:
	 *
	 * using packing;
	 * std::cin >> unpack() >> value;
	 * std::cout << pack() << value;
	 */
	typedef packing::packer_type<packer> pack;
	typedef packing::unpacker_type<unpacker> unpack;
	
} // namespace packing

/**
 * Begin packing on the provided output stream.
 */
template<class pack>
typename pack::type& operator<<(std::ostream &os, pack p)
{
	return p.getPacker(os);
}

/**
 * Begin unpacking from the provided input stream.
 */
template<class unpack>
typename unpack::type& operator>>(std::istream &is, unpack p)
{
	return p.getUnpacker(is);
}

/**
 * Serialize the given instance of the template type T into
 * a Packer object.
 */
template<typename T>
packing::packer& operator<<(packing::packer& p, const T& v)
{
	return p.pack(v);
}

/**
 * Deserialize from the provided Unpacker object forcing a cast into the
 * provided object reference. Unpacking results are copied into v, so
 * this is not recommended when deserializing arrays, maps or raw object
 * types is expected.
 * This may throw an exception if the Unpacker runs out of input data.
 */
template<typename T>
packing::unpacker& operator>>(packing::unpacker& u, T& v)
throw (packing::unpack_exception)
{
	packing::Object* obj = 0;
	if ((obj = u.unpack()) != 0)
	{
		v = obj->getAs<T>();
	}
	else
	{
		// TODO: rollback unpacker action?
		throw packing::unpack_exception("Unable to get object from stream");
	}
	delete obj;
}

/**
 * Print an Object instance into the provided output stream.
 */
template<typename char_t>
std::basic_ostream<char_t>& operator<<(std::basic_ostream<char_t>& s, const packing::Object& o)
{
	using namespace packing;
	
	detail::type_traits<ARRAY>::type array;
	detail::type_traits<ARRAY>::type::const_iterator arrayIt;
	detail::type_traits<MAP>::type map;
	detail::type_traits<MAP>::type::const_iterator mapIt;
	
	switch (o.getType()) {
		case NIL:
			return (s << "null");
		case BOOLEAN:
			return (s << (o.getValue<BOOLEAN>() ? "true" : "false"));
		case CHAR:
			return (s << (int) o.getValue<CHAR>());
		case SHORT:
			return (s << o.getValue<SHORT>());
		case INTEGER:
			return (s << o.getValue<INTEGER>());
		case LONG:
			return (s << o.getValue<LONG>());
		case UCHAR:
			return (s << (int) o.getValue<UCHAR>());
		case USHORT:
			return (s << o.getValue<USHORT>());
		case UINTEGER:
			return (s << o.getValue<UINTEGER>());
		case ULONG:
			return (s << o.getValue<ULONG>());
		case FLOAT:
			return (s << o.getValue<FLOAT>());
		case DOUBLE:
			return (s << o.getValue<DOUBLE>());
		case RAW:
			return (s << '"' << o.getValue<std::basic_string<char_t> >() << '"');
		case ARRAY:
			array = o.getValue<ARRAY>();
			arrayIt = array.begin();
			s << "array{";
			while (++arrayIt != array.end()) {
				s << "[" << **arrayIt++ << "]";
			}
			s << "}";
			break;
		case MAP:
			map = o.getValue<MAP>();
			mapIt = map.begin();
			s << "map{";
			while (mapIt != map.end()) {
				s << "[" << *mapIt->first << "," << *mapIt->second << "]";
				mapIt++;
			}
			s << "}";
			break;
		default:
			return (s << "[Unknown type]");
	}
	return s;
}

#ifndef MSGPACK_NAMESPACE__
#define MSGPACK_NAMESPACE__ msgpack
#endif

namespace MSGPACK_NAMESPACE__
{
	namespace bm
	{
		/*****************************************************
		 * Type identifiers as specified by:
		 * {@link http://wiki.msgpack.org/display/MSGPACK/Format+specification }
		 * ***************************************************/
		
		/*****************************************************
		 * Fixed length types
		 *****************************************************/
		
		//!< Integers
		static const unsigned char MP_INT8 = (unsigned char) 0xd0;
		static const unsigned char MP_INT16 = (unsigned char) 0xd1;
		static const unsigned char MP_INT32 = (unsigned char) 0xd2;
		static const unsigned char MP_INT64 = (unsigned char) 0xd3;
		static const unsigned char MP_UINT8 = (unsigned char) 0xcc;
		static const unsigned char MP_UINT16 = (unsigned char) 0xcd;
		static const unsigned char MP_UINT32 = (unsigned char) 0xce;
		static const unsigned char MP_UINT64 = (unsigned char) 0xcf;
		static const unsigned char MP_FIXNUM = (unsigned char) 0x00; //!< Last 7 bits is value
		static const unsigned char MP_NEGATIVE_FIXNUM = (unsigned char) 0xe0; //!< Last 5 bits is value
		
		//!< nil
		static const unsigned char MP_NULL = (unsigned char) 0xc0;
		
		//!< boolean
		static const unsigned char MP_FALSE = (unsigned char) 0xc2;
		static const unsigned char MP_TRUE = (unsigned char) 0xc3;
		
		//!< Floating point
		static const unsigned char MP_FLOAT = (unsigned char) 0xca;
		static const unsigned char MP_DOUBLE = (unsigned char) 0xcb;
		
		/*****************************************************
		 * Variable length types
		 *****************************************************/
		
		//<! Raw bytes
		static const unsigned char MP_RAW16 = (unsigned char) 0xda;
		static const unsigned char MP_RAW32 = (unsigned char) 0xdb;
		static const unsigned char MP_FIXRAW = (unsigned char) 0xa0; //!< Last 5 bits is size
		
		/*****************************************************
		 * Container types
		 *****************************************************/
		
		//!< Arrays
		static const unsigned char MP_ARRAY16 = (unsigned char) 0xdc;
		static const unsigned char MP_ARRAY32 = (unsigned char) 0xdd;
		static const unsigned char MP_FIXARRAY = (unsigned char) 0x90; //<! Lst 4 bits is size
		
		//!< Maps
		static const unsigned char MP_MAP16 = (unsigned char) 0xde;
		static const unsigned char MP_MAP32 = (unsigned char) 0xdf;
		static const unsigned char MP_FIXMAP = (unsigned char) 0x80; //<! Last 4 bits is size
		
		// Some helper bitmasks
		static const int MAX_4BIT = 0xf;
		static const int MAX_5BIT = 0x1f;
		static const int MAX_7BIT = 0x7f;
		static const int MAX_8BIT = 0xff;
		static const int MAX_15BIT = 0x7fff;
		static const int MAX_16BIT = 0xffff;
		static const int MAX_31BIT = 0x7fffffff;
		static const long MAX_32BIT = 0xffffffffL;
		
	} //namespace bm
	
	/**
	 * msgpack_packer class. Provides the functionality to serialize data
	 * into a stream using the MessagePack binary data.
	 */
	class msgpack_packer : public packing::packer
	{
	public:
		
		// Avoid name hiding to bring the parent pack overloads here
		using packer::pack;
		
		/**
		 * Constructor
		 * @param out Output stream where the binary data will be put
		 */
		explicit msgpack_packer(std::ostream& out) : packer(out) {}
		
		/******************************************************************
		 * From packing::packer (overloads)
		 ******************************************************************/
		
		packer& packNull()
		{
			return write(bm::MP_NULL);
		}
		
		packer& pack(bool item)
		{
			return (item ? write(bm::MP_TRUE) : write(bm::MP_FALSE));
		}
		
		packer& pack(int item)
		{
			return pack((long) item);
		}
		
		packer& pack(long value)
		{
			if (value >= 0)
			{
				if (value <= bm::MAX_7BIT)
				{
					write<char>((char(value) | bm::MP_FIXNUM));
				}
				else if (value <= bm::MAX_8BIT)
				{
					write(bm::MP_UINT8).write<char>(value);
				}
				else if (value <= bm::MAX_16BIT)
				{
					write(bm::MP_UINT16).write<uint16_t>(value);
				}
				else if (value <= bm::MAX_32BIT)
				{
					write(bm::MP_UINT32).write<uint32_t>(value);
				}
				else
				{
					write(bm::MP_UINT64).write<uint64_t>(value);
				}
			}
			else
			{
				if (value >= -(bm::MAX_5BIT + 1))
				{
					write<char>(char(value) | bm::MP_NEGATIVE_FIXNUM);
				}
				else if (value >= -(bm::MAX_7BIT + 1))
				{
					write(bm::MP_INT8).write<char>(value);
				}
				else if (value >= -(bm::MAX_15BIT + 1))
				{
					write(bm::MP_INT16).write<int16_t>(value);
				}
				else if (value >= -(int64_t(bm::MAX_31BIT) + 1))
				{
					write(bm::MP_INT32).write<int32_t>(value);
				}
				else
				{
					write(bm::MP_INT64).write<int64_t>(value);
				}
			}
			return *this;
		}
		
		packer& pack(float item)
		{
			return write(bm::MP_FLOAT).write(item);
		}
		
		packer& pack(double item)
		{
			return write(bm::MP_DOUBLE).write(item);
		}
		
		packer& pack(const char* data, std::size_t length)
		{
			if (length <= bm::MAX_5BIT)
			{
				write((char) (((char) length) | bm::MP_FIXRAW));
			}
			else if (length <= bm::MAX_16BIT)
			{
				write(bm::MP_RAW16).write<short>(length);
			}
			else
			{
				write(bm::MP_RAW32).write<int>(length);
			}
			write(data, length);
			return *this;
		}
		
	private:
		
		packer& packArrayHeader(std::size_t& length)
		{
			if (length <= bm::MAX_4BIT)
			{
				write<char>(((char) length) | bm::MP_FIXARRAY);
			}
			else if (length <= bm::MAX_16BIT)
			{
				write(bm::MP_ARRAY16).write<short>(length);
			}
			else
			{
				write(bm::MP_ARRAY32).write<int>(length);
			}
			return *this;
		}
		
		packer& packMapHeader(std::size_t& length)
		{
			if (length <= bm::MAX_4BIT)
			{
				write<char>(((char) length) | bm::MP_FIXMAP);
			}
			else if (length <= bm::MAX_16BIT)
			{
				write(bm::MP_MAP16).write<short>(length);
			}
			else
			{
				write(bm::MP_MAP32).write<int>(length);
			}
			return *this;
		}
		
	}; // msgpack_packer
	
	/**
	 * msgpack_unpacker class. Allows to deserialize MessagePack binary data
	 * from a provided istream object.
	 */
	class msgpack_unpacker : public packing::unpacker
	{
	public:
		
		/**
		 * Constructor
		 * @param in Input stream from where the binary data is taken
		 */
		msgpack_unpacker(std::istream& in) : packing::unpacker(in) {}
		
		/******************************************************************
		 * From packing::unpacker (overload)
		 ******************************************************************/
		
		packing::Object* unpack() throw(packing::unpack_exception)
		{
			using namespace packing::detail;
			
			unsigned char value;
			read(value); // Read the header
			
			type_traits<packing::BOOLEAN>::type boolVal;
			type_traits<packing::FLOAT>::type fVal;
			type_traits<packing::DOUBLE>::type dVal;
			type_traits<packing::CHAR>::type cVal;
			type_traits<packing::SHORT>::type sVal;
			type_traits<packing::INTEGER>::type iVal;
			type_traits<packing::LONG>::type lVal;
			type_traits<packing::UCHAR>::type ucVal;
			type_traits<packing::USHORT>::type usVal;
			type_traits<packing::UINTEGER>::type uiVal;
			type_traits<packing::ULONG>::type ulVal;
			
			switch (value)
			{
				case bm::MP_NULL:
					return new Nil();
				case bm::MP_FALSE:
					return new Bool(false);
				case bm::MP_TRUE:
					return new Bool(true);
				case bm::MP_FLOAT:
					read(fVal);
					return new Float(fVal);
				case bm::MP_DOUBLE:
					read(dVal);
					return new Double(dVal);
				case bm::MP_UINT8:
					read(ucVal);
					return new UnsignedChar(ucVal);
				case bm::MP_UINT16:
					read(usVal);
					return new UnsignedShort(usVal);
				case bm::MP_UINT32:
					read(uiVal);
					return new UnsignedInt(uiVal);
				case bm::MP_UINT64:
					read(ulVal);
					return new UnsignedLong(ulVal);
				case bm::MP_INT8:
					read(cVal);
					return new Char(cVal);
				case bm::MP_INT16:
					read(sVal);
					return new Short(sVal);
				case bm::MP_INT32:
					read(uiVal);
					return new Int(iVal);
				case bm::MP_INT64:
					read(lVal);
					return new Long(lVal);
				case bm::MP_ARRAY16:
					read(sVal);
					return unpackArray(sVal);
				case bm::MP_ARRAY32:
					read(iVal);
					return unpackArray(iVal);
				case bm::MP_MAP16:
					read(sVal);
					return unpackMap(sVal);
				case bm::MP_MAP32:
					read(iVal);
					return unpackMap(iVal);
				case bm::MP_RAW16:
					read(usVal);
					return unpackRaw(usVal);
				case bm::MP_RAW32:
					read(iVal);
					return unpackRaw(iVal);
			}
			
			if (((unsigned char)(value & 0xE0)) == bm::MP_FIXRAW)
			{
				return unpackRaw(value - bm::MP_FIXRAW);
			}
			
			if (((unsigned char)(value & 0xE0)) == bm::MP_NEGATIVE_FIXNUM)
			{
				return new Int((value & 0x1F) - 32);
			}
			
			if (((unsigned char)(value & 0xF0)) == bm::MP_FIXARRAY)
			{
				return unpackArray(value - bm::MP_FIXARRAY);
			}
			
			if (((unsigned char)(value & 0xF0)) == bm::MP_FIXMAP)
			{
				return unpackMap(value - bm::MP_FIXMAP);
			}
			
			if (value <= 127) //MP_FIXNUM
			{
				return new Char(value);
			}
			else
			{
				return 0;
			}
		}
		
	}; //msgpack_unpacker
	
	// Define the packer and unpacker classes for the current
	// protocol implementation and namespace
	typedef msgpack_packer packer;
	typedef msgpack_unpacker unpacker;
	
	// Define the pack and unpack helpers for the current
	// namespace
	typedef packing::packer_type<packer> pack;
	typedef packing::unpacker_type<unpacker> unpack;
	
} // namespace MSGPACK_NAMESPACE__

#endif // MSGPACK_HPP_