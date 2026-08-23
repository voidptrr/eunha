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

use std::error::Error;
use std::fmt::{self, Display, Formatter};
use std::net::IpAddr;

use super::ServerConfig;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ServerConfigBuildError {
    MissingAddress,
    MissingPort,
}

impl Display for ServerConfigBuildError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        match self {
            Self::MissingAddress => formatter.write_str("server address is required"),
            Self::MissingPort => formatter.write_str("server port is required"),
        }
    }
}

impl Error for ServerConfigBuildError {}

#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct ServerConfigBuilder {
    address: Option<IpAddr>,
    port: Option<u16>,
}

impl ServerConfigBuilder {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn address(&mut self, address: IpAddr) -> &mut Self {
        self.address = Some(address);
        self
    }

    pub fn port(&mut self, port: u16) -> &mut Self {
        self.port = Some(port);
        self
    }

    pub fn build(self) -> Result<ServerConfig, ServerConfigBuildError> {
        Ok(ServerConfig {
            address: self.address.ok_or(ServerConfigBuildError::MissingAddress)?,
            port: self.port.ok_or(ServerConfigBuildError::MissingPort)?,
        })
    }
}

#[cfg(test)]
mod tests {
    use std::net::{IpAddr, Ipv4Addr};

    use super::{ServerConfigBuildError, ServerConfigBuilder};

    #[test]
    fn builds_a_server_configuration() {
        let address = IpAddr::V4(Ipv4Addr::LOCALHOST);
        let mut builder = ServerConfigBuilder::new();
        builder.address(address).port(8080);
        let config = builder.build().expect("complete config should build");

        assert_eq!(config.address(), address);
        assert_eq!(config.port(), 8080);
    }

    #[test]
    fn reports_each_missing_required_field() {
        assert_eq!(
            ServerConfigBuilder::new().build(),
            Err(ServerConfigBuildError::MissingAddress)
        );

        let mut builder = ServerConfigBuilder::new();
        builder.address(IpAddr::V4(Ipv4Addr::LOCALHOST));
        assert_eq!(builder.build(), Err(ServerConfigBuildError::MissingPort));
    }

    #[test]
    fn repeated_setters_replace_previous_values() {
        let mut builder = ServerConfigBuilder::new();
        builder
            .address(IpAddr::V4(Ipv4Addr::UNSPECIFIED))
            .address(IpAddr::V4(Ipv4Addr::LOCALHOST))
            .port(80)
            .port(8080);
        let config = builder.build().expect("complete config should build");

        assert_eq!(config.address(), IpAddr::V4(Ipv4Addr::LOCALHOST));
        assert_eq!(config.port(), 8080);
    }
}
