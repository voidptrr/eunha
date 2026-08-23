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

use std::collections::{HashMap, hash_map::Entry};

use super::{HeaderName, HeaderValue};

/// HTTP headers grouped by their validated, case-insensitive field name.
#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct HeaderMap {
    pub(super) entries: HashMap<HeaderName, Vec<HeaderValue>>,
}

impl HeaderMap {
    pub fn new() -> Self {
        Self::default()
    }

    /// Inserts a new name or adds another value to an existing name.
    pub fn upsert(&mut self, name: HeaderName, value: HeaderValue) {
        match self.entries.entry(name) {
            Entry::Occupied(mut entry) => entry.get_mut().push(value),
            Entry::Vacant(entry) => {
                entry.insert(vec![value]);
            }
        }
    }

    /// Returns every value stored under the field name.
    pub fn get(&self, name: &HeaderName) -> Option<&[HeaderValue]> {
        self.entries.get(name).map(Vec::as_slice)
    }

    /// Returns the number of distinct field names in the map.
    pub fn len(&self) -> usize {
        self.entries.len()
    }

    pub fn is_empty(&self) -> bool {
        self.entries.is_empty()
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
    fn upsert_groups_values_under_a_normalized_name() {
        let mut headers = HeaderMap::new();
        headers.upsert(name("Set-Cookie"), value("first=1"));
        headers.upsert(name("set-cookie"), value("second=2"));

        let values = headers
            .get(&name("SET-COOKIE"))
            .expect("header should exist");
        assert_eq!(values.len(), 2);
        assert_eq!(values[0].as_bytes(), b"first=1");
        assert_eq!(values[1].as_bytes(), b"second=2");
        assert_eq!(headers.len(), 1);
    }
}
