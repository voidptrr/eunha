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

mod method;
mod version;
mod builder;

use crate::header::HeaderMap;

pub use builder::{RequestBuildError, RequestBuilder};
pub use method::Method;
pub use version::Version;

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
