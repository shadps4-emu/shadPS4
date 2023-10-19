# Result
This is an adaption of [https://github.com/oktal/result](https://github.com/oktal/result). Make sure to support the original library!

## Overview

`Result<T, E>` is a template type that can be used to return and propage errors. It can be used to replace
exceptions in context where they are not allowed or too slow to be used. `Result<T, E>` is an algebraic data
type of `Ok(T)` that represents success and `Err(E)` representing an error.

Design of this class has been mainly inspired by Rust's [std::result](https://doc.rust-lang.org/std/result/)

```

struct Request {
};

struct Error {

    enum class Kind {
        Timeout,
        Invalid,
        TooLong
    }

    Error(Kind kind, std::string text);

    Kind kind;
    std::string text;
};

Result<Request, Error> parseRequest(const std::string& payload) {
    if (payload.size() > 512) return Err(Error(Kind::TooLong, "Request exceeded maximum allowed size (512 bytes)"));

    Request request;
    return Ok(request);
}

std::string payload = receivePayload();
auto request = parseRequest(payload).expect("Failed to parse request");
```

To return a successfull `Result`, use the `Ok()` function. To return an error one, use the `Err()` function.

## Extract and unwrap

To extract the value from a `Result<T, E>` type, you can use the `expect()` function that will yield the value
of an `Ok(T)` or terminate the program with an error message passed as a parameter.

```
Result<uint32_t, uint32_t> r1 = Ok(3u);

auto val = r1.expect("Failed to retrieve the value");
assert(val == 3);
```

`unwrap()` can also be used to extract the value of a `Result`, yielding the value of an `Ok(T)` value or terminating
the program otherwise:

```
Result<uint32_t, uint32_t> r1 = Ok(3u);

auto val = r1.unwrap();
assert(val == 3);
```

Instead a terminating the program, `unwrapOr` can be used to return a default value for an `Err(E)` Result:

```
Result<uint32_t, uint32_t> r1 = Err(9u);

auto val = r1.unwrapOr(0);
assert(val == 0);
```

## Map and bind

To transform (or map) a `Result<T, E>` to a `Result<U, E>`, `Result` provides a `map` member function.
`map` will apply a function to a contained `Ok(T)` value and will return the result of the transformation,
and will leave an `Err(E)` untouched:

```
std::string stringify(int val) { return std::to_string(val); }

Result<uint32_t, uint32_t> r1 = Ok(2u);
auto r2 = r1.map(stringify); // Maps a Result<uint32_t, uint32_t> to Result<std::string, uint32_t>

assert(r2.unwrap(), "2");
```

Note that `map` should return a simple value and not a `Result<U, E>`. A function returning nothing (`void`)
applied to a `Result<T, E>` will yield a `Result<void, E>`.

To map a function to a contained `Err(E)` value, use the `mapError` function.

To *bind* a `Result<T, E>` to a `Result<U, E>`, you can use the `andThen` member function:

```
Result<uint32_t, uint32_t> square(uint32_t val) { return Ok(val * val); }

Result<uint32_t, uint32_t> r1 = Ok(3u);
auto r2 = r1.andThen(square);

assert(r2.unwrap(), 9);
```

Use `orElse` to apply a function to a contained `Err(E)` value:

```
Result<uint32_t, uint32_t> identity(uint32_t val) { return Ok(val); }

Result<uint32_t, uint32_t> r1 = Err(3u);
assert(r1.andThen(identity).orElse(square).unwrap(), 9);
```

## The TRY macro

Like Rust, a `TRY` macro is also provided that comes in handy when writing code that calls a lot of functions returning a `Result`.

the `TRY` macro will simply call its argument and short-cirtcuit the function returning an `Err(E)` if the operation returned an error `Result`:

```
Result<void, IoError> copy(int srcFd, const char* dstFile) {

    auto fd = TRY(open(dstFile));
    auto data = TRY(read(srcFd));
    TRY(write(fd, data));

    return Ok();
}
```

Note that this macro uses a special extension called *compound statement* only supported by gcc and clang
