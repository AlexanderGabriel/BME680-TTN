function decodeFloat32(bytes) {
  var sign = (bytes & 0x80000000) ? -1 : 1;
  var exponent = ((bytes >> 23) & 0xFF) - 127;
  var significand = (bytes & ~(-1 << 23));

  if (exponent == 128)
      return sign * ((significand) ? Number.NaN : Number.POSITIVE_INFINITY);

  if (exponent == -127) {
      if (significand == 0) return sign * 0.0;
      exponent = -126;
      significand /= (1 << 22);
  } else significand = (significand | (1 << 23)) / (1 << 23);

  return sign * significand * Math.pow(2, exponent);
}

function decodeInt16(bytes) {
  if ((bytes & 1 << 15) > 0) { // value is negative (16bit 2's complement)
      bytes = ((~bytes) & 0xffff) + 1; // invert 16bits & add 1 => now positive value
      bytes = bytes * -1;
  }
  return bytes;
}

function int16_LE(bytes, idx) {
  bytes = bytes.slice(idx || 0);
  return bytes[0] << 0 | bytes[1] << 8;
}

function int32_LE(bytes, idx) {
  bytes = bytes.slice(idx || 0);
  return bytes[0] << 0 | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24;
}

function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  // (array) of bytes to an object of fields.
  var decoded = {
      co2: decodeFloat32(int32_LE(bytes, 0)),
  };

  return decoded;
}