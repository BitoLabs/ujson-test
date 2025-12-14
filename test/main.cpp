#include "ujson.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <array>

#define EXPECT_ERR(statement, _Err, _line)                                \
    do {                                                                  \
        SCOPED_TRACE("");                                                 \
        try {                                                             \
            statement;                                                    \
            ADD_FAILURE() << "expected ujson::Err exception";             \
        }                                                                 \
        catch (const _Err& e) {                                           \
            int32_t line = _line;                                         \
            if (line > 0) {                                               \
                EXPECT_EQ(line, e.line) << "incorrect error line number"; \
            }                                                             \
        }                                                                 \
        catch (...) {                                                     \
            ADD_FAILURE() << "expected ujson::Err exception";             \
        }                                                                 \
    } while (0)

TEST(null, all)
{
    ujson::Json json;
    EXPECT_EQ(json.parse("null").get_type(), ujson::vtNull);
}

TEST(bool, all)
{
    ujson::Json json;
    EXPECT_EQ(json.parse("false").as_bool().get(), false);
    EXPECT_EQ(json.parse("true ").as_bool().get(), true );
}

TEST(num, int)
{
    ujson::Json  json;

    // i64:
    EXPECT_EQ   (json.parse(" 42").as_int().get(),  42);
    EXPECT_EQ   (json.parse("-42").as_int().get(), -42);
    EXPECT_EQ   (json.parse(" 9223372036854775807").as_int().get(), INT64_MAX);
    EXPECT_THROW(json.parse(" 9223372036854775808"), ujson::ErrSyntax); // INT64_MAX + 1
    EXPECT_EQ   (json.parse("-9223372036854775808").as_int().get(), INT64_MIN);
    EXPECT_THROW(json.parse("-9223372036854775809"), ujson::ErrSyntax); // INT64_MIN - 1
    EXPECT_THROW(json.parse(" 01"), ujson::ErrSyntax); // a number can't start with 0 if it is followed by another digit
    EXPECT_THROW(json.parse("- 1"), ujson::ErrSyntax); // '-' must be followed by a digit
    EXPECT_THROW(json.parse(" +1"), ujson::ErrSyntax); // '+' can't precede a number
    EXPECT_THROW(json.parse("256").as_int().get(0, 255),       ujson::ErrBadIntRange);

    // i32:
    EXPECT_THROW(json.parse("256").as_int().get_i32(0, 255),   ujson::ErrBadIntRange);
    EXPECT_THROW(json.parse("21474836470").as_int().get_i32(), ujson::ErrBadIntRange);

    // u32:
    EXPECT_EQ   (json.parse("4294967295").as_int().get_u32(),  UINT32_MAX);
    EXPECT_THROW(json.parse("21474836470").as_int().get_u32(), ujson::ErrBadIntRange);
    EXPECT_THROW(json.parse("-1").as_int().get_u32(),          ujson::ErrBadIntRange);
}

TEST(num, f64)
{
    ujson::Json      json;
    EXPECT_DOUBLE_EQ(json.parse(" 42.42" ).as_f64().get(), +42.42);
    EXPECT_DOUBLE_EQ(json.parse("-42.42" ).as_f64().get(), -42.42);
    EXPECT_DOUBLE_EQ(json.parse("  0.42" ).as_f64().get(), +0.42);
    EXPECT_DOUBLE_EQ(json.parse("  1.E3" ).as_f64().get(), 1000.0);
    EXPECT_DOUBLE_EQ(json.parse("  1.e3" ).as_f64().get(), 1000.0);
    EXPECT_DOUBLE_EQ(json.parse("  1.1e3").as_f64().get(), 1100.0);
    EXPECT_DOUBLE_EQ(json.parse("100"    ).as_f64().get(),  100.0);
    EXPECT_THROW    (json.parse(" 42.0"  ).as_f64().get(100.0, 200.0), ujson::ErrBadF64Range);
    EXPECT_THROW    (json.parse(" 00.42"    ), ujson::ErrSyntax); // a number can't start with 0 if it is followed by another digit
    EXPECT_THROW    (json.parse("1e99999999"), ujson::ErrSyntax); // number too huge
}

TEST(str, plain)
{
    ujson::Json  json;
    EXPECT_STREQ(json.parse(R"("value")").as_str().get(), "value");

    // Allowed escape characters:
    EXPECT_STREQ(json.parse("\"\\\"\"").as_str().get(), "\"");
    EXPECT_STREQ(json.parse("\"\\\\\"").as_str().get(), "\\");
    EXPECT_STREQ(json.parse("\"\\/\"" ).as_str().get(), "/" );
    EXPECT_STREQ(json.parse("\"\\b\"" ).as_str().get(), "\b");
    EXPECT_STREQ(json.parse("\"\\f\"" ).as_str().get(), "\f");
    EXPECT_STREQ(json.parse("\"\\n\"" ).as_str().get(), "\n");
    EXPECT_STREQ(json.parse("\"\\r\"" ).as_str().get(), "\r");
    EXPECT_STREQ(json.parse("\"\\t\"" ).as_str().get(), "\t");

    EXPECT_THROW(json.parse("\"\\p\""), ujson::ErrSyntax); // bad esacpe character
    EXPECT_THROW(json.parse("\"\t\"" ), ujson::ErrSyntax); // no control characters allowed inside string
    EXPECT_THROW(json.parse("\"\n\"" ), ujson::ErrSyntax); // no control characters allowed inside string
    EXPECT_THROW(json.parse("\"\r\"" ), ujson::ErrSyntax); // no control characters allowed inside string
    EXPECT_THROW(json.parse("\"value"), ujson::ErrSyntax); // no closing quotes

    // Below test could fail when comparing 'char' values as signed integers.
    // For example:
    //
    //    char c = *m_next++;
    //    if (c < ' ') {
    //        // This may fail for c = 0x80 ... 0xFF
    //        throw ErrSyntax("invalid string syntax: control characters not allowed", m_line_count);
    //    }
    //
    EXPECT_STREQ(json.parse("\" \xF4\x8F\xBF\xBF \"").as_str().get(), " \xF4\x8F\xBF\xBF "); // non character in UTF-8 U+10FFFF
}

TEST(str, encoding)
{
    ujson::Json  json;
    EXPECT_STREQ(json.parse(R"("\u000A")").as_str().get(), "\n");
    EXPECT_STREQ(json.parse(R"("\u000d")").as_str().get(), "\r");
    EXPECT_STREQ(json.parse(R"("\u0020")").as_str().get(), " ");
    EXPECT_STREQ(json.parse(R"("\u007A")").as_str().get(), "z");
    EXPECT_STREQ(json.parse(R"("\u007F")").as_str().get(), "\x7F");         // (control-007F)
    EXPECT_STREQ(json.parse("\"\\u0080\"").as_str().get(), "\xC2\x80");     // (control-0080)
    EXPECT_STREQ(json.parse("\"\\u00B5\"").as_str().get(), "\xC2\xB5");     // µ (MICRO SIGN)
    EXPECT_STREQ(json.parse("\"\\u07FF\"").as_str().get(), "\xDF\xBF");     // (UNDEFINED)
    EXPECT_STREQ(json.parse("\"\\u0800\"").as_str().get(), "\xE0\xA0\x80"); // (SAMARITAN LETTER ALAF)
    EXPECT_STREQ(json.parse("\"\\u1000\"").as_str().get(), "\xE1\x80\x80"); // (MYANMAR LETTER KA)
    EXPECT_STREQ(json.parse("\"\\u20AC\"").as_str().get(), "\xE2\x82\xAC"); // ˆ (EURO SIGN)
    EXPECT_STREQ(json.parse("\"\\uD83D\\uDE02\"").as_str().get(), "\xF0\x9F\x98\x82"); // (FACE WITH TEARS OF JOY)
    EXPECT_THROW(json.parse("\"\\u\""    ), ujson::ErrSyntax); // 4 hex digits expected
    EXPECT_THROW(json.parse("\"\\u012 \""), ujson::ErrSyntax); // 4 hex digits expected
    EXPECT_THROW(json.parse("\"\\uD83D\""), ujson::ErrSyntax); // low surrogate not specified
    EXPECT_THROW(json.parse("\"\\uDC00\\uDC00\""), ujson::ErrSyntax); // high surrogate not in range (0xD800 ... 0xDBFF)
    EXPECT_THROW(json.parse("\"\\uDC00\\uDFFF\""), ujson::ErrSyntax); // high surrogate not in range (0xD800 ... 0xDBFF)
    EXPECT_THROW(json.parse("\"\\uD83D\\uDBFF\""), ujson::ErrSyntax); //  low surrogate not in range (0xDC00 ... 0xDFFF)
    EXPECT_THROW(json.parse("\"\\uD83D\\uC000\""), ujson::ErrSyntax); //  low surrogate not in range (0xDC00 ... 0xDFFF)
}

TEST(str, enum)
{

    ujson::Json json;

    std::array set { "zero", "one", "two", "three"};
    EXPECT_EQ   (json.parse(R"("two" )").as_str().get_enum_idx(set.data(), set.size()), 2);
    EXPECT_THROW(json.parse(R"("four")").as_str().get_enum_idx(set.data(), set.size()), ujson::ErrBadEnum);

    enum Color { red, green, blue };
    EXPECT_EQ(json.parse(R"("green")").as_str().get_enum(
        std::array{"red", "green", "blue"}, std::array{red, green, blue}),
        green);
}

TEST(arr, syntax)
{
    ujson::Json json;
    EXPECT_THROW(json.parse("[1 2]"),  ujson::ErrSyntax); // no comma
    EXPECT_THROW(json.parse("[1,,2]"), ujson::ErrSyntax); // no element between commas
    EXPECT_THROW(json.parse("[1,2"),   ujson::ErrSyntax); // no ']'
    EXPECT_EQ(json.parse("[1,2]").as_arr().get_len(), 2);
    EXPECT_EQ(json.parse("[1,2,]").as_arr().get_len(), 2);  // trailing comma is ok
    EXPECT_EQ(json.parse(" [ 1, 2 , ] ").as_arr().get_len(), 2); // whitespace is ok
}

TEST(arr, get_len)
{
    ujson::Json json;
    EXPECT_EQ(json.parse("[]"          ).as_arr().get_len(), 0);
    EXPECT_EQ(json.parse("[null]"      ).as_arr().get_len(), 1);
    EXPECT_EQ(json.parse("[null, null]").as_arr().get_len(), 2);
}

TEST(arr, require_len)
{
    ujson::Json json;
    EXPECT_NO_THROW(json.parse("[null, null]").as_arr().require_len(2));
    EXPECT_NO_THROW(json.parse("[null, null]").as_arr().require_len(0, 4));
    EXPECT_ERR     (json.parse("[null, null]").as_arr().require_len(1),    ujson::ErrBadArrLen, 1);
    EXPECT_ERR     (json.parse("[null, null]").as_arr().require_len(0, 1), ujson::ErrBadArrLen, 1);
    EXPECT_ERR     (json.parse("[null, null]").as_arr().require_len(4, 8), ujson::ErrBadArrLen, 1);
}

TEST(arr, get_element)
{
    ujson::Json json;
    auto& arr = json.parse("[null, false]").as_arr();
    ASSERT_EQ(arr.get_len(), 2);
    EXPECT_EQ(arr.get_element(0).get_type(), ujson::vtNull);
    EXPECT_EQ(arr.get_element(1).get_type(), ujson::vtBool);
    EXPECT_THROW(arr.get_element(100), std::out_of_range);
}

TEST(arr, get_bool)
{
    ujson::Json json;
    auto& arr = json.parse("[false,true,null]").as_arr();
    EXPECT_EQ(arr.get_bool(0), false);
    EXPECT_EQ(arr.get_bool(1), true);
    EXPECT_THROW(arr.get_bool(2), ujson::ErrBadType);
}

TEST(arr, get_i32)
{
    ujson::Json json;
    auto& arr = json.parse("[256, 21474836470, null]").as_arr();
    EXPECT_EQ   (arr.get_i32(0), 256);
    EXPECT_THROW(arr.get_i32(0, 0, 255), ujson::ErrBadIntRange);
    EXPECT_THROW(arr.get_i32(1)        , ujson::ErrBadIntRange);
    EXPECT_THROW(arr.get_i32(2)        , ujson::ErrBadType);
}

TEST(arr, get_u32)
{
    ujson::Json json;
    auto& arr = json.parse("[256, 21474836470, null]").as_arr();
    EXPECT_EQ(arr.get_u32(0), 256u);
    EXPECT_THROW(arr.get_u32(0, 0, 255), ujson::ErrBadIntRange);
    EXPECT_THROW(arr.get_u32(1), ujson::ErrBadIntRange);
    EXPECT_THROW(arr.get_u32(2), ujson::ErrBadType);
}

TEST(arr, get_i64)
{
    ujson::Json json;
    auto& arr = json.parse("[256, 21474836470, null]").as_arr();
    EXPECT_EQ   (arr.get_i64(0), 256);
    EXPECT_THROW(arr.get_i64(0, 0, 255), ujson::ErrBadIntRange);
    EXPECT_EQ   (arr.get_i64(1), 21474836470);
    EXPECT_THROW(arr.get_i64(2),         ujson::ErrBadType);
}

TEST(arr, get_f64)
{
    ujson::Json json;
    auto& arr = json.parse("[3.14, 42, null]").as_arr();
    EXPECT_DOUBLE_EQ(arr.get_f64(0), 3.14);
    EXPECT_THROW(arr.get_f64(0, 10, 100), ujson::ErrBadF64Range);
    EXPECT_DOUBLE_EQ(arr.get_f64(1), 42);
    EXPECT_THROW(arr.get_f64(2),          ujson::ErrBadType);
}

TEST(arr, get_str)
{
    ujson::Json json;
    auto& arr = json.parse(R"(["one","two",null])").as_arr();
    EXPECT_STREQ(arr.get_str(0), "one");
    EXPECT_STREQ(arr.get_str(1), "two");
    EXPECT_THROW(arr.get_str(2), ujson::ErrBadType);
}

TEST(arr, get_arr)
{
    ujson::Json json;
    auto& arr = json.parse("[[1, 2, 3], null]").as_arr();
    ASSERT_EQ(arr.get_arr(0).get_len(), 3);
    EXPECT_THROW(arr.get_arr(1), ujson::ErrBadType);
}

TEST(arr, get_obj)
{
    ujson::Json json;
    auto& arr = json.parse("[{}, null]").as_arr();
    ASSERT_EQ(arr.get_obj(0).get_len(), 0);
    EXPECT_THROW(arr.get_obj(1), ujson::ErrBadType);
}

TEST(obj, syntax)
{
    ujson::Json json;
    EXPECT_THROW(json.parse(R"({"foo":1 "bar":2})"),  ujson::ErrSyntax); // no comma
    EXPECT_THROW(json.parse(R"({"foo":1,,"bar":2})"), ujson::ErrSyntax); // no member between commas
    EXPECT_THROW(json.parse(R"({"foo":1,"bar":2)"),   ujson::ErrSyntax); // no '}'
    EXPECT_EQ(json.parse(R"({"foo":1,"bar":2})").as_obj().get_len(), 2);
    EXPECT_EQ(json.parse(R"({"foo":1,"bar":2,})").as_obj().get_len(), 2);  // trailing comma is ok
    EXPECT_EQ(json.parse(R"( { "foo" : 1 , "bar" : 2 } )").as_obj().get_len(), 2); // whitespace is ok
    EXPECT_EQ(json.parse(R"({})").as_obj().get_len(), 0); // empty obj is ok
}

TEST(obj, get_member_idx)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":1, "bar":2})").as_obj();
    EXPECT_EQ(obj.get_member_idx("foo"), 0);
    EXPECT_EQ(obj.get_member_idx("bar"), 1);
    EXPECT_EQ(obj.get_member_idx("absent", false), -1);
    EXPECT_THROW(obj.get_member_idx("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_member_name)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":1, "bar":2})").as_obj();
    EXPECT_STREQ(obj.get_member_name(0), "foo");
    EXPECT_STREQ(obj.get_member_name(1), "bar");
    EXPECT_THROW(obj.get_member_name(100), std::out_of_range);
}

TEST(obj, get_member)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":1, "bar":null})").as_obj();
    EXPECT_EQ(obj.get_member("foo")->get_type(), ujson::vtInt);
    EXPECT_EQ(obj.get_member("bar")->get_type(), ujson::vtNull);
    EXPECT_EQ(obj.get_member("absent", false), nullptr);
    EXPECT_THROW(obj.get_member("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, duplicates)
{
    ujson::Json json;
    EXPECT_THROW(json.parse(R"({"foo":1,"foo":2})"), ujson::ErrSyntax);  // duplicate member not allowed by default

    auto& obj = json.parse(R"({"foo":1,"foo":2})", 0, ujson::optStandard).as_obj(); // duplicate member allowed by standard
    EXPECT_STREQ(obj.get_member_name(0), "foo");
    EXPECT_STREQ(obj.get_member_name(1), ""); // duplicate member has no name
    EXPECT_EQ(obj.get_i32("foo"), 1); // first member can be accessed by name
    EXPECT_EQ(obj.get_element(1).as_int().get_i32(), 2); // duplicate member can be only accessed by index
}

TEST(obj, get_bool)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":false, "bar":true, "baz":null})").as_obj();
    EXPECT_EQ(obj.get_bool("foo"), false);
    EXPECT_EQ(obj.get_bool("bar"), true);
    EXPECT_THROW(obj.get_bool("baz"), ujson::ErrBadType);
    EXPECT_EQ(obj.get_bool("absent", false), false);
    EXPECT_EQ(obj.get_bool("absent", true), true);
    EXPECT_THROW(obj.get_bool("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_i32)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":42, "bar":21474836470, "baz":null})").as_obj();
    EXPECT_EQ(obj.get_i32("foo"), 42);
    EXPECT_THROW(obj.get_i32("foo", 100, 200), ujson::ErrBadIntRange);
    EXPECT_THROW(obj.get_i32("bar"), ujson::ErrBadIntRange);
    EXPECT_THROW(obj.get_i32("baz"), ujson::ErrBadType);
    EXPECT_EQ(obj.get_i32("absent", 0, -1, 123), 123);
    EXPECT_THROW(obj.get_i32("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_u32)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":42, "bar":21474836470, "baz":null})").as_obj();
    EXPECT_EQ(obj.get_u32("foo"), 42u);
    EXPECT_THROW(obj.get_u32("foo", 100, 200), ujson::ErrBadIntRange);
    EXPECT_THROW(obj.get_u32("bar"), ujson::ErrBadIntRange);
    EXPECT_THROW(obj.get_u32("baz"), ujson::ErrBadType);
    EXPECT_EQ(obj.get_u32("absent", 1, 0, 123), 123u);
    EXPECT_THROW(obj.get_u32("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_i64)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":42, "bar":21474836470, "baz":null})").as_obj();
    EXPECT_EQ(obj.get_i64("foo"), 42);
    EXPECT_THROW(obj.get_i64("foo", 100, 200), ujson::ErrBadIntRange);
    EXPECT_EQ(obj.get_i64("bar"), 21474836470);
    EXPECT_THROW(obj.get_i64("baz"), ujson::ErrBadType);
    EXPECT_EQ(obj.get_i64("absent", 0, -1, 123), 123);
    EXPECT_THROW(obj.get_i64("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_f64)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":3.14, "bar":42, "baz":null})").as_obj();
    EXPECT_DOUBLE_EQ(obj.get_f64("foo"), 3.14);
    EXPECT_THROW(obj.get_f64("foo", 100, 200), ujson::ErrBadF64Range);
    EXPECT_DOUBLE_EQ(obj.get_f64("bar"), 42);
    EXPECT_THROW(obj.get_f64("baz"), ujson::ErrBadType);
    EXPECT_DOUBLE_EQ(obj.get_f64("absent", 0, -1, 123), 123);
    EXPECT_THROW(obj.get_f64("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_str)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":"one", "bar":"two", "baz":null})").as_obj();
    EXPECT_STREQ(obj.get_str("foo"), "one");
    EXPECT_STREQ(obj.get_str("bar"), "two");
    EXPECT_THROW(obj.get_str("baz"), ujson::ErrBadType);
    EXPECT_STREQ(obj.get_str("absent", "default"), "default");
    EXPECT_THROW(obj.get_str("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, str_enum)
{
    enum Color { red, green, blue };
    const std::array str_set{"red", "green", "blue"};
    const std::array val_set{red, green, blue};

    ujson::Json json;
    auto& obj = json.parse(R"({"foo": "green", "bar": "yellow"})").as_obj();
    EXPECT_EQ(green, obj.get_str_enum("foo", str_set, val_set));
    EXPECT_EQ(green, obj.get_str_enum("baz", str_set, val_set, green));
    EXPECT_THROW(    obj.get_str_enum("bar", str_set, val_set), ujson::ErrBadEnum);
    EXPECT_THROW(    obj.get_str_enum("baz", str_set, val_set), ujson::ErrMemberNotFound);
}

TEST(obj, get_arr)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":[1,2,3], "baz":null})").as_obj();
    ASSERT_EQ(obj.get_arr("foo").get_len(), 3);
    EXPECT_THROW(obj.get_arr("baz"), ujson::ErrBadType);
    EXPECT_THROW(obj.get_arr("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_arr_opt)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":[1,2,3], "baz":null})").as_obj();
    EXPECT_NE(obj.get_arr_opt("foo"), nullptr);
    EXPECT_EQ(obj.get_arr_opt("absent"), nullptr);
    EXPECT_THROW(obj.get_arr_opt("baz"), ujson::ErrBadType);
    if (auto foo = obj.get_arr_opt("foo")) {
        EXPECT_EQ(foo->get_len(), 3);
    }
    else {
        FAIL();
    }
}

TEST(obj, get_obj)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":{}, "baz":null})").as_obj();
    ASSERT_EQ(obj.get_obj("foo").get_len(), 0);
    EXPECT_THROW(obj.get_obj("baz"), ujson::ErrBadType);
    EXPECT_THROW(obj.get_obj("absent"), ujson::ErrMemberNotFound);
}

TEST(obj, get_obj_opt)
{
    ujson::Json json;
    auto& obj = json.parse(R"({"foo":{}, "baz":null})").as_obj();
    EXPECT_NE(obj.get_obj_opt("foo"), nullptr);
    EXPECT_EQ(obj.get_obj_opt("absent"), nullptr);
    EXPECT_THROW(obj.get_obj_opt("baz"), ujson::ErrBadType);
    if (auto foo = obj.get_obj_opt("foo")) {
        EXPECT_EQ(foo->get_len(), 0);
    }
    else {
        FAIL();
    }
}

TEST(obj, comments)
{
    const char in_str[] =
    R"(// comment
    { // comment
        "foo" : 1, // comment
        "bar" : 2, // comment
      //"baz" : 3, // comment
    } // comment
    )";
    ujson::Json json;
    EXPECT_EQ(json.parse(in_str).as_obj().get_len(), 2);
}

TEST(obj, composite)
{
    const char in_str[] =
    R"({
        "name"  : "Main Window",
        "width" : 640,
        "height": 480,
        "on_top": false,
        "opacity": 0.9, // where 1.0 is fully opaque
        "menu"  : ["Open", "Save", "Exit"],
        "widgets"  : [
            { "type": "button", "name": "OK" },
            { "type": "button", "name": "Cancel" },
        ],
        "color_rgb": [0, 0, 255],
    })";

    ujson::Json json;
    const ujson::Obj& root = json.parse(in_str).as_obj();

    EXPECT_STREQ    (root.get_str("name"), "Main Window");
    EXPECT_EQ       (root.get_i32("width", 0, 16384), 640);
    EXPECT_EQ       (root.get_bool("on_top", false), false);
    EXPECT_DOUBLE_EQ(root.get_f64("opacity", 0.0, 1.0, 1.0), 0.9);

    // menu
    const ujson::Arr& menu = root.get_arr("menu");
    ASSERT_EQ(menu.get_len(), 3);
    EXPECT_STREQ(menu.get_str(0), "Open");
    EXPECT_STREQ(menu.get_str(1), "Save");
    EXPECT_STREQ(menu.get_str(2), "Exit");

    // widgets
    const ujson::Arr& widgets = root.get_arr("widgets");
    ASSERT_EQ(widgets.get_len(), 2);
    {
        const ujson::Obj& item = widgets.get_obj(0);
        EXPECT_STREQ(item.get_str("type"), "button");
        EXPECT_STREQ(item.get_str("name"), "OK");
    }
    {
        const ujson::Obj& item = widgets.get_obj(1);
        EXPECT_STREQ(item.get_str("type"), "button");
        EXPECT_STREQ(item.get_str("name"), "Cancel");
    }

    // color_rgb
    const ujson::Arr& color_rgb = root.get_arr("color_rgb");
    ASSERT_EQ(color_rgb.get_len(), 3);
    EXPECT_EQ(color_rgb.get_i32(0, 0, 255), 0);
    EXPECT_EQ(color_rgb.get_i32(1, 0, 255), 0);
    EXPECT_EQ(color_rgb.get_i32(2, 0, 255), 255);
}

TEST(val, get_line)
{
    const char in_str[] =
    R"(                   // 01
    {                     // 02
        "num" : 1,        // 03
        "arr" :           // 04
        [                 // 05
            2,            // 06
            {"foo":42}    // 07
        ],                // 08
        "obj":            // 09
            {"foo":       // 10
                1         // 11
            },            // 12
    })";

    ujson::Json json;
    const ujson::Obj& root = json.parse(in_str).as_obj();
    auto& arr = root.get_arr("arr");
    auto& obj = root.get_obj("obj");

    EXPECT_EQ(root.get_line(), 2);
    EXPECT_EQ(root.get_member("num")->get_line(), 3);
    EXPECT_EQ(arr.get_line(), 5);
    EXPECT_EQ(arr.get_element(0).get_line(), 6);
    EXPECT_EQ(arr.get_element(1).get_line(), 7);
    EXPECT_EQ(obj.get_line(), 10);
    EXPECT_EQ(obj.get_member("foo")->get_line(), 11);
}

TEST(val, reject_unknown_member)
{
    const char in_str[] =
    R"({
        "num" : 1,
        "arr" : [
            2,
            {"foo":42}
        ],
        "ignore": {"foo": 1},
    })";
    ujson::Json json;

    const ujson::Obj& root = json.parse(in_str).as_obj();

    root.get_obj("ignore").ignore_members();
    EXPECT_ERR(root.reject_unknow_members(), ujson::ErrUnknownMember, 2);

    root.get_i32("num");
    EXPECT_ERR(root.reject_unknow_members(), ujson::ErrUnknownMember, 3);

    auto& arr = root.get_arr("arr");
    EXPECT_ERR(root.reject_unknow_members(), ujson::ErrUnknownMember, 5);

    arr.get_obj(1).get_i32("foo");
    EXPECT_NO_THROW(root.reject_unknow_members());
}

TEST(json, extra_text)
{
    ujson::Json json;
    EXPECT_THROW(json.parse("1 invalid text at the end"), ujson::ErrSyntax);
}

TEST(json, nested_level)
{
    ujson::Json json;
    EXPECT_NO_THROW(json.parse("[[[[[[[[[[[[[[[[ ]]]]]]]]]]]]]]]]")); // 16 nested arrays must be ok
    {
        // Generate a string with 'n' nested arrays: [[[[ ... ]]]]
        // Must not crash but raise ErrSyntax due to too deep nested level.
        const size_t n = 1024;
        std::string s(n * 2, ']');
        s.replace(0, n, n, '[');
        EXPECT_THROW(json.parse(s.c_str()), ujson::ErrSyntax);
    }
    EXPECT_NO_THROW(json.parse(R"({"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{ }}}}}}}}}}}}}}}})")); // 16 nested objects must be ok
    {
        // Generate a string with 'n' nested structures: {{{{ ... }}}}
        // Must not crash but raise ErrSyntax due to too deep nested level.
        const size_t n = 1024;
        std::string s = "{";
        for (size_t i = 0; i < n-1; i++) {
            s.append(R"("a":{)");
        }
        for (size_t i = 0; i < n; i++) {
            s.append("}");
        }
        EXPECT_THROW(json.parse(s.c_str()), ujson::ErrSyntax);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    int errCode = RUN_ALL_TESTS();
    return errCode;
}
