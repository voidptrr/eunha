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

use std::collections::hash_map;

use super::HeaderMap;
use crate::header::{HeaderName, HeaderValue};

/// A borrowed iterator over the entries in a [`HeaderMap`].
pub struct HeaderMapIter<'a> {
    inner: hash_map::Iter<'a, HeaderName, Vec<HeaderValue>>,
}

impl<'a> Iterator for HeaderMapIter<'a> {
    type Item = (&'a HeaderName, &'a [HeaderValue]);

    fn next(&mut self) -> Option<Self::Item> {
        self.inner
            .next()
            .map(|(name, values)| (name, values.as_slice()))
    }
}

impl<'a> IntoIterator for &'a HeaderMap {
    type Item = (&'a HeaderName, &'a [HeaderValue]);
    type IntoIter = HeaderMapIter<'a>;

    fn into_iter(self) -> Self::IntoIter {
        HeaderMapIter {
            inner: self.entries.iter(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{HeaderMap, HeaderName, HeaderValue};

    fn name(value: &str) -> HeaderName {
        HeaderName::try_from(value.as_bytes().to_vec()).expect("test header name should be valid")
    }

    fn value(value: &str) -> HeaderValue {
        HeaderValue::try_from(value.as_bytes().to_vec()).expect("test header value should be valid")
    }

    #[test]
    fn supports_borrowed_iteration() {
        let mut headers = HeaderMap::new();
        headers.upsert(name("Accept"), value("text/plain"));
        headers.upsert(name("Accept"), value("text/html"));

        let entries: Vec<_> = (&headers).into_iter().collect();
        assert_eq!(entries.len(), 1);
        assert_eq!(entries[0].0.as_str(), "accept");
        assert_eq!(entries[0].1.len(), 2);
    }
}
