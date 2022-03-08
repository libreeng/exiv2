// SPDX-License-Identifier: GPL-2.0-or-later

// included header files
#include "value.hpp"

#include "convert.hpp"
#include "enforce.hpp"
#include "error.hpp"
#include "types.hpp"

// + standard includes

#include <regex>
#include <sstream>

// *****************************************************************************
// class member definitions
namespace Exiv2 {

    Value::Value(TypeId typeId)
        : ok_(true), type_(typeId)
    {
    }

    Value::UniquePtr Value::create(TypeId typeId)
    {
        std::unique_ptr<Value> value;
        switch (typeId) {
        case invalidTypeId:
        case signedByte:
        case unsignedByte:
            value = std::make_unique<DataValue>(typeId);
            break;
        case asciiString:
            value = std::make_unique<AsciiValue>();
            break;
        case unsignedShort:
            value = std::make_unique<ValueType<uint16_t>>();
            break;
        case unsignedLong:
        case tiffIfd:
            value = std::make_unique<ValueType<uint32_t>>(typeId);
            break;
        case unsignedRational:
            value = std::make_unique<ValueType<URational>>();
            break;
        case undefined:
            value = std::make_unique<DataValue>();
            break;
        case signedShort:
            value = std::make_unique<ValueType<int16_t>>();
            break;
        case signedLong:
            value = std::make_unique<ValueType<int32_t>>();
            break;
        case signedRational:
            value = std::make_unique<ValueType<Rational>>();
            break;
        case tiffFloat:
            value = std::make_unique<ValueType<float>>();
            break;
        case tiffDouble:
            value = std::make_unique<ValueType<double>>();
            break;
        case string:
            value = std::make_unique<StringValue>();
            break;
        case date:
            value = std::make_unique<DateValue>();
            break;
        case time:
            value = std::make_unique<TimeValue>();
            break;
        case comment:
            value = std::make_unique<CommentValue>();
            break;
        case xmpText:
            value = std::make_unique<XmpTextValue>();
            break;
        case xmpBag:
        case xmpSeq:
        case xmpAlt:
            value = std::make_unique<XmpArrayValue>(typeId);
            break;
        case langAlt:
            value = std::make_unique<LangAltValue>();
            break;
        default:
            value = std::make_unique<DataValue>(typeId);
            break;
        }
        return value;
    } // Value::create

    int Value::setDataArea(const byte* /*buf*/, size_t /*len*/)
    {
        return -1;
    }

    std::string Value::toString() const
    {
        std::ostringstream os;
        write(os);
        ok_ = !os.fail();
        return os.str();
    }

    std::string Value::toString(size_t /*n*/) const
    {
        return toString();
    }

    size_t Value::sizeDataArea() const
    {
        return 0;
    }

    DataBuf Value::dataArea() const
    {
        return {nullptr, 0};
    }

    DataValue::DataValue(TypeId typeId)
        : Value(typeId)
    {
    }

    DataValue::DataValue(const byte* buf,
              long len, ByteOrder byteOrder,TypeId typeId)
        : Value(typeId)
    {
        read(buf, len, byteOrder);
    }

    size_t DataValue::count() const
    {
        return size();
    }

    int DataValue::read(const byte* buf, size_t len, ByteOrder /*byteOrder*/)
    {
        // byteOrder not needed
        value_.assign(buf, buf + len);
        return 0;
    }

    int DataValue::read(const std::string& buf)
    {
        std::istringstream is(buf);
        int tmp = 0;
        ValueType val;
        while (!(is.eof())) {
            is >> tmp;
            if (is.fail()) return 1;
            val.push_back(static_cast<byte>(tmp));
        }
        value_.swap(val);
        return 0;
    }

    long DataValue::copy(byte* buf, ByteOrder /*byteOrder*/) const
    {
        // byteOrder not needed
        return static_cast<long>(std::copy(value_.begin(), value_.end(), buf) - buf);
    }

    size_t DataValue::size() const
    {
        return value_.size();
    }

    DataValue* DataValue::clone_() const
    {
        return new DataValue(*this);
    }

    std::ostream& DataValue::write(std::ostream& os) const
    {
        std::vector<byte>::size_type end = value_.size();
        for (std::vector<byte>::size_type i = 0; i != end; ++i) {
            os << static_cast<int>(value_.at(i));
            if (i < end - 1) os << " ";
        }
        return os;
    }

    std::string DataValue::toString(size_t n) const
    {
        std::ostringstream os;
        os << static_cast<int>(value_.at(n));
        ok_ = !os.fail();
        return os.str();
    }

    int64_t DataValue::toInt64(size_t n) const
    {
        ok_ = true;
        return value_.at(n);
    }

    uint32_t DataValue::toUint32(size_t n) const
    {
        ok_ = true;
        return value_.at(n);
    }

    float DataValue::toFloat(size_t n) const
    {
        ok_ = true;
        return value_.at(n);
    }

    Rational DataValue::toRational(size_t n) const
    {
        ok_ = true;
        return {value_.at(n), 1};
    }

    StringValueBase::StringValueBase(TypeId typeId)
        : Value(typeId)
    {
    }

    StringValueBase::StringValueBase(TypeId typeId, const std::string& buf)
        : Value(typeId)
    {
        read(buf);
    }

    StringValueBase& StringValueBase::operator=(const StringValueBase& rhs)
    {
        if (this == &rhs) return *this;
        Value::operator=(rhs);
        value_ = rhs.value_;
        return *this;
    }

    int StringValueBase::read(const std::string& buf)
    {
        value_ = buf;
        return 0;
    }

    int StringValueBase::read(const byte* buf, size_t len, ByteOrder /*byteOrder*/)
    {
        // byteOrder not needed
        if (buf) value_ = std::string(reinterpret_cast<const char*>(buf), len);
        return 0;
    }

    long StringValueBase::copy(byte* buf, ByteOrder /*byteOrder*/) const
    {
        if (value_.empty())
            return 0;
        // byteOrder not needed
        assert(buf);
        return static_cast<long>(
            value_.copy(reinterpret_cast<char*>(buf), value_.size())
            );
    }

    size_t StringValueBase::count() const
    {
        return size();
    }

    size_t StringValueBase::size() const
    {
        return value_.size();
    }

    std::ostream& StringValueBase::write(std::ostream& os) const
    {
        return os << value_;
    }

    int64_t StringValueBase::toInt64(size_t n) const
    {
        ok_ = true;
        return value_.at(n);
    }

    uint32_t StringValueBase::toUint32(size_t n) const
    {
        ok_ = true;
        return value_.at(n);
    }

    float StringValueBase::toFloat(size_t n) const
    {
        ok_ = true;
        return value_.at(n);
    }

    Rational StringValueBase::toRational(size_t n) const
    {
        ok_ = true;
        return {value_.at(n), 1};
    }

    StringValue::StringValue()
        : StringValueBase(string)
    {
    }

    StringValue::StringValue(const std::string& buf)
        : StringValueBase(string, buf)
    {
    }

    StringValue* StringValue::clone_() const
    {
        return new StringValue(*this);
    }

    AsciiValue::AsciiValue()
        : StringValueBase(asciiString)
    {
    }

    AsciiValue::AsciiValue(const std::string& buf)
        : StringValueBase(asciiString, buf)
    {
    }

    int AsciiValue::read(const std::string& buf)
    {
        value_ = buf;
        // ensure count>0 and nul terminated # https://github.com/Exiv2/exiv2/issues/1484
        if (value_.empty() || value_.at(value_.size() - 1) != '\0') {
            value_ += '\0';
        }
        return 0;
    }

    AsciiValue* AsciiValue::clone_() const
    {
        return new AsciiValue(*this);
    }

    std::ostream& AsciiValue::write(std::ostream& os) const
    {
        // Write only up to the first '\0' (if any)
        std::string::size_type pos = value_.find_first_of('\0');
        if (pos == std::string::npos) pos = value_.size();
        return os << value_.substr(0, pos);
    }

    CommentValue::CharsetTable::CharsetTable(CharsetId charsetId,
                                             const char* name,
                                             const char* code)
        : charsetId_(charsetId), name_(name), code_(code)
    {
    }

    //! Lookup list of supported IFD type information
    const CommentValue::CharsetTable CommentValue::CharsetInfo::charsetTable_[] = {
        CharsetTable(ascii,            "Ascii",            "ASCII\0\0\0"),
        CharsetTable(jis,              "Jis",              "JIS\0\0\0\0\0"),
        CharsetTable(unicode,          "Unicode",          "UNICODE\0"),
        CharsetTable(undefined,        "Undefined",        "\0\0\0\0\0\0\0\0"),
        CharsetTable(invalidCharsetId, "InvalidCharsetId", "\0\0\0\0\0\0\0\0"),
        CharsetTable(lastCharsetId,    "InvalidCharsetId", "\0\0\0\0\0\0\0\0")
    };

    const char* CommentValue::CharsetInfo::name(CharsetId charsetId)
    {
        return charsetTable_[ charsetId < lastCharsetId ? charsetId : undefined ].name_;
    }

    const char* CommentValue::CharsetInfo::code(CharsetId charsetId)
    {
        return charsetTable_[ charsetId < lastCharsetId ? charsetId : undefined ].code_;
    }

    CommentValue::CharsetId CommentValue::CharsetInfo::charsetIdByName(
        const std::string& name)
    {
        int i = 0;
        for (;    charsetTable_[i].charsetId_ != lastCharsetId
               && charsetTable_[i].name_ != name; ++i) {}
        return charsetTable_[i].charsetId_ == lastCharsetId ?
               invalidCharsetId : charsetTable_[i].charsetId_;
    }

    CommentValue::CharsetId CommentValue::CharsetInfo::charsetIdByCode(
        const std::string& code)
    {
        int i = 0;
        for (;    charsetTable_[i].charsetId_ != lastCharsetId
               && std::string(charsetTable_[i].code_, 8) != code; ++i) {}
        return charsetTable_[i].charsetId_ == lastCharsetId ?
               invalidCharsetId : charsetTable_[i].charsetId_;
    }

    CommentValue::CommentValue()
        : StringValueBase(Exiv2::undefined), byteOrder_(littleEndian)
    {
    }

    CommentValue::CommentValue(const std::string& comment)
        : StringValueBase(Exiv2::undefined), byteOrder_(littleEndian)
    {
        read(comment);
    }

    int CommentValue::read(const std::string& comment)
    {
        std::string c = comment;
        CharsetId charsetId = undefined;
        if (comment.length() > 8 && comment.substr(0, 8) == "charset=") {
            const std::string::size_type pos = comment.find_first_of(' ');
            std::string name = comment.substr(8, pos-8);
            // Strip quotes (so you can also specify the charset without quotes)
            if (!name.empty() && name[0] == '"') name = name.substr(1);
            if (!name.empty() && name[name.length()-1] == '"') name = name.substr(0, name.length()-1);
            charsetId = CharsetInfo::charsetIdByName(name);
            if (charsetId == invalidCharsetId) {
#ifndef SUPPRESS_WARNINGS
                EXV_WARNING << Error(ErrorCode::kerInvalidCharset, name) << "\n";
#endif
                return 1;
            }
            c.clear();
            if (pos != std::string::npos) c = comment.substr(pos+1);
        }
        if (charsetId == unicode) {
            const char* to = byteOrder_ == littleEndian ? "UCS-2LE" : "UCS-2BE";
            convertStringCharset(c, "UTF-8", to);
        }
        const std::string code(CharsetInfo::code(charsetId), 8);
        return StringValueBase::read(code + c);
    }

    int CommentValue::read(const byte* buf, size_t len, ByteOrder byteOrder)
    {
        byteOrder_ = byteOrder;
        return StringValueBase::read(buf, len, byteOrder);
    }

    long CommentValue::copy(byte* buf, ByteOrder byteOrder) const
    {
        std::string c = value_;
        if (charsetId() == unicode) {
            c = value_.substr(8);
            [[maybe_unused]] const size_t sz = c.size();
            if (byteOrder_ == littleEndian && byteOrder == bigEndian) {
                convertStringCharset(c, "UCS-2LE", "UCS-2BE");
                assert(c.size() == sz);
            }
            else if (byteOrder_ == bigEndian && byteOrder == littleEndian) {
                convertStringCharset(c, "UCS-2BE", "UCS-2LE");
                assert(c.size() == sz);
            }
            c = value_.substr(0, 8) + c;
        }
        if (c.empty())
            return 0;
        assert(buf);
        return static_cast<long>(c.copy(reinterpret_cast<char*>(buf), c.size()));
    }

    std::ostream& CommentValue::write(std::ostream& os) const
    {
        CharsetId csId = charsetId();
        if (csId != undefined) {
            os << "charset=" << CharsetInfo::name(csId) << " ";
        }
        return os << comment();
    }

    std::string CommentValue::comment(const char* encoding) const
    {
        std::string c;
        if (value_.length() < 8) {
            return c;
        }
        c = value_.substr(8);
        if (charsetId() == unicode) {
            const char* from = !encoding || *encoding == '\0' ? detectCharset(c) : encoding;
            convertStringCharset(c, from, "UTF-8");
        }
        bool bAscii = charsetId() == undefined || charsetId() == ascii ;
        // # 1266 Remove trailing nulls
        if ( bAscii && c.find('\0') != std::string::npos) {
            c = c.substr(0,c.find('\0'));
        }
        return c;
    }

    CommentValue::CharsetId CommentValue::charsetId() const
    {
        CharsetId charsetId = undefined;
        if (value_.length() >= 8) {
            const std::string code = value_.substr(0, 8);
            charsetId = CharsetInfo::charsetIdByCode(code);
        }
        return charsetId;
    }

    const char* CommentValue::detectCharset(std::string& c) const
    {
        // Interpret a BOM if there is one
        if (0 == strncmp(c.data(), "\xef\xbb\xbf", 3)) {
            c = c.substr(3);
            return "UTF-8";
        }
        if (0 == strncmp(c.data(), "\xff\xfe", 2)) {
            c = c.substr(2);
            return "UCS-2LE";
        }
        if (0 == strncmp(c.data(), "\xfe\xff", 2)) {
            c = c.substr(2);
            return "UCS-2BE";
        }

        // Todo: Add logic to guess if the comment is encoded in UTF-8

        return byteOrder_ == littleEndian ? "UCS-2LE" : "UCS-2BE";
    }

    CommentValue* CommentValue::clone_() const
    {
        return new CommentValue(*this);
    }

    XmpValue::XmpValue(TypeId typeId)
        : Value(typeId),
          xmpArrayType_(xaNone),
          xmpStruct_(xsNone)
    {
    }

    void XmpValue::setXmpArrayType(XmpArrayType xmpArrayType)
    {
        xmpArrayType_ = xmpArrayType;
    }

    void XmpValue::setXmpStruct(XmpStruct xmpStruct)
    {
        xmpStruct_ = xmpStruct;
    }

    XmpValue::XmpArrayType XmpValue::xmpArrayType() const
    {
        return xmpArrayType_;
    }

    XmpValue::XmpArrayType XmpValue::xmpArrayType(TypeId typeId)
    {
        XmpArrayType xa = xaNone;
        switch (typeId) {
        case xmpAlt: xa = xaAlt; break;
        case xmpBag: xa = xaBag; break;
        case xmpSeq: xa = xaSeq; break;
        default: break;
        }
        return xa;
    }

    XmpValue::XmpStruct XmpValue::xmpStruct() const
    {
        return xmpStruct_;
    }

    long XmpValue::copy(byte* buf,
                        ByteOrder /*byteOrder*/) const
    {
        std::ostringstream os;
        write(os);
        std::string s = os.str();
        if (!s.empty())
            std::memcpy(buf, &s[0], s.size());
        return static_cast<long>(s.size());
    }

    int XmpValue::read(const byte* buf, size_t len, ByteOrder /*byteOrder*/)
    {
        std::string s(reinterpret_cast<const char*>(buf), len);
        return read(s);
    }

    size_t XmpValue::size() const
    {
        std::ostringstream os;
        write(os);
        return os.str().size();
    }

    XmpTextValue::XmpTextValue()
        : XmpValue(xmpText)
    {
    }

    XmpTextValue::XmpTextValue(const std::string& buf)
        : XmpValue(xmpText)
    {
        read(buf);
    }

    int XmpTextValue::read(const std::string& buf)
    {
        // support a type=Alt,Bag,Seq,Struct indicator
        std::string b = buf;
        std::string type;
        if (buf.length() > 5 && buf.substr(0, 5) == "type=") {
            std::string::size_type pos = buf.find_first_of(' ');
            type = buf.substr(5, pos-5);
            // Strip quotes (so you can also specify the type without quotes)
            if (!type.empty() && type[0] == '"') type = type.substr(1);
            if (!type.empty() && type[type.length()-1] == '"') type = type.substr(0, type.length()-1);
            b.clear();
            if (pos != std::string::npos) b = buf.substr(pos+1);
        }
        if (!type.empty()) {
            if (type == "Alt") {
                setXmpArrayType(XmpValue::xaAlt);
            }
            else if (type == "Bag") {
                setXmpArrayType(XmpValue::xaBag);
            }
            else if (type == "Seq") {
                setXmpArrayType(XmpValue::xaSeq);
            }
            else if (type == "Struct") {
                setXmpStruct();
            }
            else {
                throw Error(ErrorCode::kerInvalidXmpText, type);
            }
        }
        value_ = b;
        return 0;
    }

    XmpTextValue::UniquePtr XmpTextValue::clone() const
    {
        return UniquePtr(clone_());
    }

    size_t XmpTextValue::size() const
    {
        return value_.size();
    }

    size_t XmpTextValue::count() const
    {
        return size();
    }

    std::ostream& XmpTextValue::write(std::ostream& os) const
    {
        bool del = false;
        if (xmpArrayType() != XmpValue::xaNone) {
            switch (xmpArrayType()) {
            case XmpValue::xaAlt: os << "type=\"Alt\""; break;
            case XmpValue::xaBag: os << "type=\"Bag\""; break;
            case XmpValue::xaSeq: os << "type=\"Seq\""; break;
            case XmpValue::xaNone: break; // just to suppress the warning
            }
            del = true;
        }
        else if (xmpStruct() != XmpValue::xsNone) {
            switch (xmpStruct()) {
            case XmpValue::xsStruct: os << "type=\"Struct\""; break;
            case XmpValue::xsNone: break; // just to suppress the warning
            }
            del = true;
        }
        if (del && !value_.empty()) os << " ";
        return os << value_;
    }

    int64_t XmpTextValue::toInt64(size_t /*n*/) const
    {
        return parseInt64(value_, ok_);
    }

    uint32_t XmpTextValue::toUint32(size_t /*n*/) const
    {
        return parseUint32(value_, ok_);
    }

    float XmpTextValue::toFloat(size_t /*n*/) const
    {
        return parseFloat(value_, ok_);
    }

    Rational XmpTextValue::toRational(size_t /*n*/) const
    {
        return parseRational(value_, ok_);
    }

    XmpTextValue* XmpTextValue::clone_() const
    {
        return new XmpTextValue(*this);
    }

    XmpArrayValue::XmpArrayValue(TypeId typeId)
        : XmpValue(typeId)
    {
        setXmpArrayType(xmpArrayType(typeId));
    }

    int XmpArrayValue::read(const std::string& buf)
    {
        if (!buf.empty()) value_.push_back(buf);
        return 0;
    }

    XmpArrayValue::UniquePtr XmpArrayValue::clone() const
    {
        return UniquePtr(clone_());
    }

    size_t XmpArrayValue::count() const
    {
        return value_.size();
    }

    std::ostream& XmpArrayValue::write(std::ostream& os) const
    {
        for (auto i = value_.begin(); i != value_.end(); ++i) {
            if (i != value_.begin()) os << ", ";
            os << *i;
        }
        return os;
    }

    std::string XmpArrayValue::toString(size_t n) const
    {
        ok_ = true;
        return value_.at(n);
    }

    int64_t XmpArrayValue::toInt64(size_t n) const
    {
        return parseInt64(value_.at(n), ok_);
    }

    uint32_t XmpArrayValue::toUint32(size_t n) const
    {
        return parseUint32(value_.at(n), ok_);
    }

    float XmpArrayValue::toFloat(size_t n) const
    {
        return parseFloat(value_.at(n), ok_);
    }

    Rational XmpArrayValue::toRational(size_t n) const
    {
        return parseRational(value_.at(n), ok_);
    }

    XmpArrayValue* XmpArrayValue::clone_() const
    {
        return new XmpArrayValue(*this);
    }

    LangAltValue::LangAltValue()
        : XmpValue(langAlt)
    {
    }

    LangAltValue::LangAltValue(const std::string& buf)
        : XmpValue(langAlt)
    {
        read(buf);
    }

    int LangAltValue::read(const std::string& buf)
    {
        std::string b = buf;
        std::string lang = "x-default";
        if (buf.length() > 5 && buf.substr(0, 5) == "lang=") {
            static const char* ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

            const std::string::size_type pos = buf.find_first_of(' ');
            if (pos == std::string::npos) {
                lang = buf.substr(5);
            } else {
                lang = buf.substr(5, pos-5);
            }
            if (lang.empty()) throw Error(ErrorCode::kerInvalidLangAltValue, buf);
            // Strip quotes (so you can also specify the language without quotes)
            if (lang[0] == '"') {
                lang = lang.substr(1);

                if (lang.empty() || lang.find('"') != lang.length() - 1)
                    throw Error(ErrorCode::kerInvalidLangAltValue, buf);

                lang = lang.substr(0, lang.length()-1);
            }

            if (lang.empty())
                throw Error(ErrorCode::kerInvalidLangAltValue, buf);

            // Check language is in the correct format (see https://www.ietf.org/rfc/rfc3066.txt)
            std::string::size_type charPos = lang.find_first_not_of(ALPHA);
            if (charPos != std::string::npos) {
                static const char* ALPHA_NUM = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
                if (lang.at(charPos) != '-' || lang.find_first_not_of(ALPHA_NUM, charPos+1) != std::string::npos)
                    throw Error(ErrorCode::kerInvalidLangAltValue, buf);
            }

            b.clear();
            if (pos != std::string::npos) b = buf.substr(pos+1);
        }

        value_[lang] = b;
        return 0;
    }

    LangAltValue::UniquePtr LangAltValue::clone() const
    {
        return UniquePtr(clone_());
    }

    size_t LangAltValue::count() const
    {
        return value_.size();
    }

    static constexpr auto x_default = "x-default";

    std::ostream& LangAltValue::write(std::ostream& os) const
    {
        bool        first     = true;

        // Write the default entry first
        auto i = value_.find(x_default);
        if (i != value_.end()) {
            os << "lang=\"" << i->first << "\" " << i->second;
            first = false;
        }

        // Write the others
        for (auto&& v : value_) {
            if (v.first != x_default) {
                if (!first) os << ", ";
                os << "lang=\"" << v.first << "\" " << v.second;
                first = false;
            }
        }
        return os;
    }

    std::string LangAltValue::toString(size_t /*n*/) const
    {
        return toString(x_default);
    }

    std::string LangAltValue::toString(const std::string& qualifier) const
    {
        auto i = value_.find(qualifier);
        if (i != value_.end()) {
            ok_ = true;
            return i->second;
        }
        ok_ = false;
        return "";
    }

    int64_t LangAltValue::toInt64(size_t /*n*/) const
    {
        ok_ = false;
        return 0;
    }

    uint32_t LangAltValue::toUint32(size_t /*n*/) const
    {
        ok_ = false;
        return 0;
    }

    float LangAltValue::toFloat(size_t /*n*/) const
    {
        ok_ = false;
        return 0.0F;
    }

    Rational LangAltValue::toRational(size_t /*n*/) const
    {
        ok_ = false;
        return {0, 0};
    }

    LangAltValue* LangAltValue::clone_() const
    {
        return new LangAltValue(*this);
    }

    DateValue::DateValue()
        : Value(date)
    {
    }

    DateValue::DateValue(int year, int month, int day)
        : Value(date)
    {
        date_.year = year;
        date_.month = month;
        date_.day = day;
    }

    int DateValue::read(const byte* buf, size_t len, ByteOrder /*byteOrder*/)
    {
        const std::string str(reinterpret_cast<const char*>(buf), len);
        return read(str);
    }

    int DateValue::read(const std::string& buf)
    {
        // ISO 8601 date formats:
        // https://web.archive.org/web/20171020084445/https://www.loc.gov/standards/datetime/ISO_DIS%208601-1.pdf
        static const std::regex reExtended(R"(^(\d{4})-(\d{2})-(\d{2}))");
        static const std::regex reBasic(R"(^(\d{4})(\d{2})(\d{2}))");
        std::smatch sm;

        auto printWarning = [](){
#ifndef SUPPRESS_WARNINGS
          EXV_WARNING << Error(ErrorCode::kerUnsupportedDateFormat) << "\n";
#endif
        };

        // Note: We use here regex_search instead of regex_match, because the string can be longer than expected and
        // also contain the time
        if (std::regex_search(buf, sm, reExtended) || std::regex_search(buf, sm, reBasic)) {
            date_.year = std::stoi(sm[1].str());
            date_.month = std::stoi(sm[2].str());
            if (date_.month > 12) {
                date_.month = 0;
                printWarning();
                return 1;
            }
            date_.day = std::stoi(sm[3].str());
            if (date_.day > 31) {
                date_.day = 0;
                printWarning();
                return 1;
            }
            return 0;
        }
        printWarning();
        return 1;
    }

    void DateValue::setDate(const Date& src)
    {
        date_.year = src.year;
        date_.month = src.month;
        date_.day = src.day;
    }

    long DateValue::copy(byte* buf, ByteOrder /*byteOrder*/) const
    {
        // \note Here the date is copied in the Basic format YYYYMMDD, as the IPTC key	Iptc.Application2.DateCreated
        // wants it. Check https://exiv2.org/iptc.html

        // sprintf wants to add the null terminator, so use oversized buffer
        char temp[9];
        int wrote = snprintf(temp, sizeof(temp), "%04d%02d%02d", date_.year, date_.month, date_.day);
        assert(wrote == 8);
        std::memcpy(buf, temp, wrote);
        return wrote;
    }

    const DateValue::Date& DateValue::getDate() const
    {
        return date_;
    }

    size_t DateValue::count() const
    {
        return size();
    }

    size_t DateValue::size() const
    {
        return 8;
    }

    DateValue* DateValue::clone_() const
    {
        return new DateValue(*this);
    }

    std::ostream& DateValue::write(std::ostream& os) const
    {
        // Write DateValue in ISO 8601 Extended format: YYYY-MM-DD
        std::ios::fmtflags f( os.flags() );
        os << std::setw(4) << std::setfill('0') << date_.year << '-' << std::right
           << std::setw(2) << std::setfill('0') << date_.month << '-'
           << std::setw(2) << std::setfill('0') << date_.day;
        os.flags(f);
        return os;
    }

    int64_t DateValue::toInt64(size_t /*n*/) const
    {
        // Range of tm struct is limited to about 1970 to 2038
        // This will return -1 if outside that range
        std::tm tms;
        std::memset(&tms, 0, sizeof(tms));
        tms.tm_mday = date_.day;
        tms.tm_mon = date_.month - 1;
        tms.tm_year = date_.year - 1900;
        auto l = static_cast<int64_t>(std::mktime(&tms));
        ok_ = (l != -1);
        return l;
    }

    uint32_t DateValue::toUint32(size_t /*n*/) const
    {
        const int64_t t = toInt64();
        if (t < 0 || t > std::numeric_limits<uint32_t>::max()) {
            return 0;
        }
        return static_cast<uint32_t>(t);
    }

    float DateValue::toFloat(size_t n) const
    {
        return static_cast<float>(toInt64(n));
    }

    Rational DateValue::toRational(size_t n) const
    {
        return {static_cast<int32_t>(toInt64(n)), 1};
    }

    TimeValue::TimeValue()
        : Value(time)
    {
    }

    TimeValue::TimeValue(int hour, int minute,
                         int second, int tzHour,
                         int tzMinute)
        : Value(date)
    {
        time_.hour = hour;
        time_.minute = minute;
        time_.second = second;
        time_.tzHour = tzHour;
        time_.tzMinute = tzMinute;
    }

    int TimeValue::read(const byte* buf, size_t len, ByteOrder /*byteOrder*/)
    {
        const std::string str(reinterpret_cast<const char*>(buf), len);
        return read(str);
    }

    int TimeValue::read(const std::string& buf)
    {
        // ISO 8601 time formats:
        // https://web.archive.org/web/20171020084445/https://www.loc.gov/standards/datetime/ISO_DIS%208601-1.pdf
        // Not supported formats:
        // 4.2.2.4 Representations with decimal fraction: 232050,5
        static const std::regex re(R"(^(2[0-3]|[01][0-9]):?([0-5][0-9])?:?([0-5][0-9])?$)");
        static const std::regex reExt(R"(^(2[0-3]|[01][0-9]):?([0-5][0-9]):?([0-5][0-9])(Z|[+-](?:2[0-3]|[01][0-9])(?::?(?:[0-5][0-9]))?)$)");

        std::smatch sm;
        if (std::regex_match(buf, sm, re) || std::regex_match(buf, sm, reExt)) {
          time_.hour = sm.length(1) ? std::stoi(sm[1].str()) : 0;
          time_.minute = sm.length(2) ? std::stoi(sm[2].str()) : 0;
          time_.second = sm.length(3) ? std::stoi(sm[3].str()) : 0;
          if (sm.size() > 4)
          {
            std::string str = sm[4].str();
            const auto strSize = str.size();
            auto posColon = str.find(':');

            if (posColon == std::string::npos) {
                // Extended format
                time_.tzHour =   std::stoi(str.substr(0,3));
                if (strSize > 3) {
                  int minute = std::stoi(str.substr(3));
                  time_.tzMinute = time_.tzHour < 0 ? -minute : minute;
                }
            } else {
                // Basic format
                time_.tzHour = std::stoi(str.substr(0, posColon));
                int minute = std::stoi(str.substr(posColon+1));
                time_.tzMinute = time_.tzHour < 0 ? -minute : minute;
            }
          }
          return 0;
        }
#ifndef SUPPRESS_WARNINGS
        EXV_WARNING << Error(ErrorCode::kerUnsupportedTimeFormat) << "\n";
#endif
        return 1;
    }

    /// \todo not used internally. At least we should test it
    void TimeValue::setTime( const Time& src )
    {
        std::memcpy(&time_, &src, sizeof(time_));
    }

    long TimeValue::copy(byte* buf, ByteOrder /*byteOrder*/) const
    {
        // NOTE: Here the time is copied in the Basic format HHMMSS:HHMM, as the IPTC key	Iptc.Application2.TimeCreated
        // wants it. Check https://exiv2.org/iptc.html
        char temp[12];
        char plusMinus = '+';
        if (time_.tzHour < 0 || time_.tzMinute < 0)
            plusMinus = '-';

        const int wrote = snprintf(temp, sizeof(temp), // 11 bytes are written + \0
                   "%02d%02d%02d%1c%02d%02d",
                   time_.hour, time_.minute, time_.second,
                   plusMinus, abs(time_.tzHour), abs(time_.tzMinute));

        enforce(wrote == 11, Exiv2::ErrorCode::kerUnsupportedTimeFormat);
        std::memcpy(buf, temp, wrote);
        return wrote;
    }

    const TimeValue::Time& TimeValue::getTime() const
    {
        return time_;
    }

    size_t TimeValue::count() const
    {
        return size();
    }

    size_t TimeValue::size() const
    {
        return 11;
    }

    TimeValue* TimeValue::clone_() const
    {
        return new TimeValue(*this);
    }

    std::ostream& TimeValue::write(std::ostream& os) const
    {
        // Write TimeValue in ISO 8601 Extended format: hh:mm:ss±hh:mm
        char plusMinus = '+';
        if (time_.tzHour < 0 || time_.tzMinute < 0)
          plusMinus = '-';

        std::ios::fmtflags f( os.flags() );
        os << std::right
           << std::setw(2) << std::setfill('0') << time_.hour << ':'
           << std::setw(2) << std::setfill('0') << time_.minute << ':'
           << std::setw(2) << std::setfill('0') << time_.second << plusMinus
           << std::setw(2) << std::setfill('0') << abs(time_.tzHour) << ':'
           << std::setw(2) << std::setfill('0') << abs(time_.tzMinute);
        os.flags(f);

        return os;
    }

    int64_t TimeValue::toInt64(size_t /*n*/) const
    {
        // Returns number of seconds in the day in UTC.
        int64_t result = (time_.hour - time_.tzHour) * 60 * 60;
        result += (time_.minute - time_.tzMinute) * 60;
        result += time_.second;
        if (result < 0) {
            result += 86400;
        }
        ok_ = true;
        return result;
    }

    uint32_t TimeValue::toUint32(size_t /*n*/) const
    {
        const int64_t t = toInt64();
        if (t < 0 || t > std::numeric_limits<uint32_t>::max()) {
            return 0;
        }
        return static_cast<uint32_t>(t);
    }

    float TimeValue::toFloat(size_t n) const
    {
        return static_cast<float>(toInt64(n));
    }

    Rational TimeValue::toRational(size_t n) const
    {
        return {static_cast<int32_t>(toInt64(n)), 1};
    }

}  // namespace Exiv2
