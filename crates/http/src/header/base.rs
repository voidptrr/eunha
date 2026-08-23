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
use std::error::Error;
use std::fmt::{self, Display, Formatter};

/// Returned when a field name is empty or contains a byte outside HTTP's token syntax.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InvalidHeaderName;
impl Error for InvalidHeaderName {}

impl Display for InvalidHeaderName {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        formatter.write_str("invalid HTTP header name")
    }
}

/// Returned when a field value contains a forbidden control byte.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InvalidHeaderValue;
impl Error for InvalidHeaderValue {}

impl Display for InvalidHeaderValue {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        formatter.write_str("invalid HTTP header value")
    }
}

/// A validated, lowercase HTTP field name.
#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub struct HeaderName(String);

impl HeaderName {
    pub fn as_str(&self) -> &str {
        &self.0
    }

    fn is_valid(name: &[u8]) -> bool {
        !name.is_empty()
            && name.iter().copied().all(|byte| {
                byte.is_ascii_alphanumeric()
                    || matches!(
                        byte,
                        b'!' | b'#'
                            | b'$'
                            | b'%'
                            | b'&'
                            | b'\''
                            | b'*'
                            | b'+'
                            | b'-'
                            | b'.'
                            | b'^'
                            | b'_'
                            | b'`'
                            | b'|'
                            | b'~'
                    )
            })
    }
}

impl TryFrom<Vec<u8>> for HeaderName {
    type Error = InvalidHeaderName;

    fn try_from(mut name: Vec<u8>) -> Result<Self, Self::Error> {
        if !Self::is_valid(&name) {
            return Err(InvalidHeaderName);
        }

        name.make_ascii_lowercase();
        let name = String::from_utf8(name).map_err(|_| InvalidHeaderName)?;
        Ok(Self(name))
    }
}

impl Display for HeaderName {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        formatter.write_str(self.as_str())
    }
}

/// An HTTP field value containing only bytes permitted by the protocol.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct HeaderValue(Vec<u8>);

impl HeaderValue {
    pub fn as_bytes(&self) -> &[u8] {
        &self.0
    }

    fn is_valid(value: &[u8]) -> bool {
        value
            .iter()
            .all(|byte| *byte == b'\t' || (b' '..=b'~').contains(byte) || *byte >= 0x80)
    }
}

impl TryFrom<Vec<u8>> for HeaderValue {
    type Error = InvalidHeaderValue;

    fn try_from(value: Vec<u8>) -> Result<Self, Self::Error> {
        if !Self::is_valid(&value) {
            return Err(InvalidHeaderValue);
        }

        Ok(Self(value))
    }
}

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
    use super::{HeaderMap, HeaderName, HeaderValue, InvalidHeaderName, InvalidHeaderValue};

    fn name(value: &str) -> HeaderName {
        HeaderName::try_from(value.as_bytes().to_vec()).expect("test header name should be valid")
    }

    fn value(value: &str) -> HeaderValue {
        HeaderValue::try_from(value.as_bytes().to_vec()).expect("test header value should be valid")
    }

    #[test]
    fn validates_and_normalizes_field_names() {
        assert_eq!(name("Content-Type").as_str(), "content-type");
        assert_eq!(name("X_Custom~Name").as_str(), "x_custom~name");

        assert_eq!(HeaderName::try_from(Vec::new()), Err(InvalidHeaderName));
        assert_eq!(
            HeaderName::try_from(b"bad name".to_vec()),
            Err(InvalidHeaderName)
        );
        assert_eq!(
            HeaderName::try_from(b"bad:name".to_vec()),
            Err(InvalidHeaderName)
        );
        assert_eq!(
            HeaderName::try_from("café".as_bytes().to_vec()),
            Err(InvalidHeaderName)
        );
    }

    #[test]
    fn validates_field_values() {
        assert!(HeaderValue::try_from(Vec::new()).is_ok());
        assert!(HeaderValue::try_from(b"text/plain\t;charset=utf-8".to_vec()).is_ok());
        assert!(HeaderValue::try_from(vec![0x80, 0xff]).is_ok());

        assert_eq!(
            HeaderValue::try_from(b"line\r\nbreak".to_vec()),
            Err(InvalidHeaderValue)
        );
        assert_eq!(HeaderValue::try_from(vec![0]), Err(InvalidHeaderValue));
        assert_eq!(HeaderValue::try_from(vec![0x7f]), Err(InvalidHeaderValue));
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
