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

use crate::header::{HeaderMap, HeaderName, HeaderValue};
use std::error::Error;
use std::fmt::{self, Display, Formatter};

/// An HTTP request method.
#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub enum Method {
    Connect,
    Delete,
    Get,
    Head,
    Options,
    Patch,
    Post,
    Put,
    Trace,
    /// A method not represented by a standard variant.
    Other(String),
}

/// An HTTP protocol version supported by Eunha.
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum Version {
    Http10,
    Http11,
}

/// A complete, owned HTTP request.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Request {
    method: Method,
    target: String,
    version: Version,
    headers: HeaderMap,
    body: Vec<u8>,
}

impl Request {
    pub fn builder() -> RequestBuilder {
        RequestBuilder::default()
    }

    pub fn method(&self) -> &Method {
        &self.method
    }

    pub fn target(&self) -> &str {
        &self.target
    }

    pub const fn version(&self) -> Version {
        self.version
    }

    pub const fn headers(&self) -> &HeaderMap {
        &self.headers
    }

    pub fn body(&self) -> &[u8] {
        &self.body
    }
}

/// Builds a request incrementally while keeping incomplete data unobservable.
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct RequestBuilder {
    method: Option<Method>,
    target: Option<String>,
    version: Option<Version>,
    headers: HeaderMap,
    body: Vec<u8>,
}

impl RequestBuilder {
    pub fn method(&mut self, method: Method) -> &mut Self {
        self.method = Some(method);
        self
    }

    pub fn target(&mut self, target: impl Into<String>) -> &mut Self {
        self.target = Some(target.into());
        self
    }

    pub fn version(&mut self, version: Version) -> &mut Self {
        self.version = Some(version);
        self
    }

    pub fn headers(&mut self, headers: HeaderMap) -> &mut Self {
        self.headers = headers;
        self
    }

    pub fn header(&mut self, name: HeaderName, value: HeaderValue) -> &mut Self {
        self.headers.upsert(name, value);
        self
    }

    pub fn body(&mut self, body: impl Into<Vec<u8>>) -> &mut Self {
        self.body = body.into();
        self
    }

    pub fn extend_body(&mut self, bytes: impl AsRef<[u8]>) -> &mut Self {
        self.body.extend_from_slice(bytes.as_ref());
        self
    }

    pub fn build(self) -> Result<Request, RequestBuildError> {
        let method = self.method.ok_or(RequestBuildError::MissingMethod)?;
        let target = self.target.ok_or(RequestBuildError::MissingTarget)?;
        let version = self.version.ok_or(RequestBuildError::MissingVersion)?;

        Ok(Request {
            method,
            target,
            version,
            headers: self.headers,
            body: self.body,
        })
    }
}

/// Identifies a required request field that was not supplied to the builder.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum RequestBuildError {
    MissingMethod,
    MissingTarget,
    MissingVersion,
}

impl Display for RequestBuildError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        match self {
            Self::MissingMethod => formatter.write_str("request method is required"),
            Self::MissingTarget => formatter.write_str("request target is required"),
            Self::MissingVersion => formatter.write_str("request version is required"),
        }
    }
}

impl Error for RequestBuildError {}

#[cfg(test)]
mod tests {
    use super::{Method, Request, RequestBuildError, Version};
    use crate::header::{HeaderMap, HeaderName, HeaderValue};

    fn header_name(value: &str) -> HeaderName {
        HeaderName::try_from(value.as_bytes().to_vec()).expect("test header name should be valid")
    }

    fn header_value(value: &str) -> HeaderValue {
        HeaderValue::try_from(value.as_bytes().to_vec()).expect("test header value should be valid")
    }

    fn complete_builder() -> super::RequestBuilder {
        let mut builder = Request::builder();
        builder
            .method(Method::Get)
            .target("/")
            .version(Version::Http11);
        builder
    }

    #[test]
    fn stores_known_and_extension_methods() {
        let mut builder = complete_builder();
        builder.method(Method::Post);
        let request = builder.build().expect("complete request should build");
        assert_eq!(request.method(), &Method::Post);

        let mut builder = complete_builder();
        builder.method(Method::Other("PURGE".to_owned()));
        let request = builder.build().expect("complete request should build");
        assert_eq!(request.method(), &Method::Other("PURGE".to_owned()));
    }

    #[test]
    fn reports_each_missing_required_field() {
        assert_eq!(
            Request::builder().build(),
            Err(RequestBuildError::MissingMethod)
        );

        let mut builder = Request::builder();
        builder.method(Method::Get);
        assert_eq!(builder.build(), Err(RequestBuildError::MissingTarget));

        let mut builder = Request::builder();
        builder.method(Method::Get).target("/");
        assert_eq!(builder.build(), Err(RequestBuildError::MissingVersion));
    }

    #[test]
    fn builds_with_empty_headers_and_body_by_default() {
        let request = complete_builder()
            .build()
            .expect("complete request should build");

        assert_eq!(request.method(), &Method::Get);
        assert_eq!(request.target(), "/");
        assert_eq!(request.version(), Version::Http11);
        assert!(request.headers().is_empty());
        assert!(request.body().is_empty());
    }

    #[test]
    fn repeated_scalar_and_bulk_setters_replace_previous_values() {
        let mut old_headers = HeaderMap::new();
        old_headers.upsert(header_name("Old"), header_value("value"));
        let mut new_headers = HeaderMap::new();
        new_headers.upsert(header_name("New"), header_value("value"));

        let mut builder = Request::builder();
        builder
            .method(Method::Get)
            .method(Method::Put)
            .target("/old")
            .target("/new")
            .version(Version::Http10)
            .version(Version::Http11)
            .headers(old_headers)
            .headers(new_headers)
            .body(b"old".to_vec())
            .body(b"new".to_vec());
        let request = builder.build().expect("complete request should build");

        assert_eq!(request.method(), &Method::Put);
        assert_eq!(request.target(), "/new");
        assert_eq!(request.version(), Version::Http11);
        assert!(request.headers().get(&header_name("Old")).is_none());
        assert!(request.headers().get(&header_name("New")).is_some());
        assert_eq!(request.body(), b"new");
    }

    #[test]
    fn extends_a_binary_body() {
        let mut builder = complete_builder();
        builder
            .body(vec![0, 1])
            .extend_body([2, 0])
            .extend_body([3]);
        let request = builder.build().expect("complete request should build");

        assert_eq!(request.body(), &[0, 1, 2, 0, 3]);
    }

    #[test]
    fn builder_upserts_headers() {
        let mut builder = complete_builder();
        builder
            .header(header_name("Accept"), header_value("text/plain"))
            .header(header_name("Accept"), header_value("text/html"));
        let request = builder.build().expect("complete request should build");

        assert_eq!(
            request
                .headers()
                .get(&header_name("accept"))
                .map(<[HeaderValue]>::len),
            Some(2)
        );
    }
}
