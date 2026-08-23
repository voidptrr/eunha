// MIT License
//
// Copyright (c) 2026 Tommaso Bruno
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

use std::fmt::{self, Display, Formatter};

/// An HTTP response status currently emitted by Eunha.
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[repr(u16)]
pub enum StatusCode {
    Ok = 200,
    Found = 302,
    BadRequest = 400,
    Unauthorized = 401,
    NotFound = 404,
    MethodNotAllowed = 405,
    ContentTooLarge = 413,
    UnsupportedMediaType = 415,
    RequestHeaderFieldsTooLarge = 431,
    InternalServerError = 500,
    NotImplemented = 501,
    HttpVersionNotSupported = 505,
}

impl StatusCode {
    pub const fn reason_phrase(self) -> &'static str {
        match self {
            Self::Ok => "OK",
            Self::Found => "Found",
            Self::BadRequest => "Bad Request",
            Self::Unauthorized => "Unauthorized",
            Self::NotFound => "Not Found",
            Self::MethodNotAllowed => "Method Not Allowed",
            Self::ContentTooLarge => "Content Too Large",
            Self::UnsupportedMediaType => "Unsupported Media Type",
            Self::RequestHeaderFieldsTooLarge => "Request Header Fields Too Large",
            Self::InternalServerError => "Internal Server Error",
            Self::NotImplemented => "Not Implemented",
            Self::HttpVersionNotSupported => "HTTP Version Not Supported",
        }
    }
}

impl From<StatusCode> for u16 {
    fn from(status: StatusCode) -> Self {
        status as Self
    }
}

impl Display for StatusCode {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        write!(formatter, "{} {}", u16::from(*self), self.reason_phrase())
    }
}

#[cfg(test)]
mod tests {
    use super::StatusCode;

    #[test]
    fn exposes_registered_codes_and_reason_phrases() {
        let cases = [
            (StatusCode::Ok, 200, "OK"),
            (StatusCode::Found, 302, "Found"),
            (StatusCode::BadRequest, 400, "Bad Request"),
            (StatusCode::Unauthorized, 401, "Unauthorized"),
            (StatusCode::NotFound, 404, "Not Found"),
            (StatusCode::MethodNotAllowed, 405, "Method Not Allowed"),
            (StatusCode::ContentTooLarge, 413, "Content Too Large"),
            (
                StatusCode::UnsupportedMediaType,
                415,
                "Unsupported Media Type",
            ),
            (
                StatusCode::RequestHeaderFieldsTooLarge,
                431,
                "Request Header Fields Too Large",
            ),
            (
                StatusCode::InternalServerError,
                500,
                "Internal Server Error",
            ),
            (StatusCode::NotImplemented, 501, "Not Implemented"),
            (
                StatusCode::HttpVersionNotSupported,
                505,
                "HTTP Version Not Supported",
            ),
        ];

        for (status, code, reason_phrase) in cases {
            assert_eq!(u16::from(status), code);
            assert_eq!(status.reason_phrase(), reason_phrase);
        }
    }

    #[test]
    fn displays_an_http_status() {
        assert_eq!(StatusCode::BadRequest.to_string(), "400 Bad Request");
    }
}
