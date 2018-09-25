#include <stdint.h>
#include <avr/pgmspace.h>

#define M(a,b) (((a)<<4) | (b))
#define XOFF(m) ((m)>>4)
#define DW(m) (m&0xF)

const uint8_t PROGMEM font_metadata[224] = {
	M( 0, 3 ), // ' ' 0x20
	M( 2, 3 ), // ! 
	M( 0, 8 ), // " 
	M( 0, 8 ), // # 
	M( 0, 8 ), // $ 
	M( 0, 8 ), // % 
	M( 0, 8 ), // & 
	M( 2, 5 ), // ' 
	M( 1, 6 ), // ( 
	M( 1, 6 ), // ) 
	M( 1, 7 ), // * 
	M( 1, 7 ), // + 
	M( 2, 5 ), // , 
	M( 1, 7 ), // - 
	M( 2, 4 ), // . 
	M( 1, 7 ), // / 
	M( 0, 8 ), // 0 0x30
	M( 0, 8 ), // 1
	M( 0, 8 ), // 2
	M( 0, 8 ), // 3
	M( 0, 8 ), // 4
	M( 0, 8 ), // 5
	M( 0, 8 ), // 6
	M( 0, 8 ), // 7
	M( 0, 8 ), // 8
	M( 0, 8 ), // 9
	M( 2, 3 ), // :
	M( 2, 3 ), // ;
	M( 1, 7 ), // <
	M( 1, 7 ), // =
	M( 1, 7 ), // >
	M( 0, 8 ), // ?
	M( 0, 8 ), // @ 0x40
	M( 0, 8 ), // A
	M( 0, 8 ), // B
	M( 0, 8 ), // C
	M( 0, 8 ), // D
	M( 0, 8 ), // E
	M( 0, 8 ), // F
	M( 0, 8 ), // G
	M( 0, 8 ), // H
	M( 0, 8 ), // I
	M( 0, 8 ), // J
	M( 0, 8 ), // K
	M( 0, 8 ), // L
	M( 0, 8 ), // M
	M( 0, 8 ), // N
	M( 0, 8 ), // O
	M( 0, 8 ), // P 0x50
	M( 0, 8 ), // Q
	M( 0, 8 ), // R
	M( 0, 8 ), // S
	M( 0, 8 ), // T
	M( 0, 8 ), // U
	M( 0, 8 ), // V
	M( 0, 8 ), // W
	M( 0, 8 ), // X
	M( 0, 8 ), // Y
	M( 0, 8 ), // Z
	M( 1, 7 ), // [
	M( 0, 8 ), // '\'
	M( 0, 7 ), // ]
	M( 0, 8 ), // ^
	M( 0, 8 ), // _
	M( 2, 4 ), // ` 0x60
	M( 0, 7 ), // a
	M( 1, 7 ), // b
	M( 1, 6 ), // c
	M( 0, 7 ), // d
	M( 0, 7 ), // e
	M( 2, 5 ), // f
	M( 0, 7 ), // g
	M( 0, 7 ), // h
	M( 1, 5 ), // i
	M( 1, 6 ), // j
	M( 1, 6 ), // k
	M( 2, 5 ), // l
	M( 0, 7 ), // m
	M( 0, 7 ), // n
	M( 0, 7 ), // o
	M( 0, 7 ), // p 0x70
	M( 0, 7 ), // q
	M( 1, 6 ), // r
	M( 0, 7 ), // s
	M( 1, 6 ), // t
	M( 0, 7 ), // u
	M( 0, 7 ), // v
	M( 0, 7 ), // w
	M( 0, 7 ), // x
	M( 0, 7 ), // y
	M( 0, 7 ), // z
	M( 1, 7 ), // M(
	M( 0, 8 ), // |
	M( 0, 7 ), // }
	M( 0, 8 ), // ~
	M( 1, 7 ), // up arrow
	M( 0, 8 ), // € 0x80
	M( 0, 8 ), // up left arrow
	M( 0, 8 ), // ´
	M( 0, 8 ), // f-like char
	M( 0, 8 ), // ¨ ... i think
	M( 0, 8 ), // ...
	M( 0, 8 ), // dagger
	M( 0, 8 ), // double dagger
	M( 0, 8 ), // ^ of some kind
	M( 0, 8 ), // %. promille
	M( 0, 8 ), // S-with-hat
	M( 1, 7 ), // < quotation
	M( 0, 8 ), // OE
	M( 0, 8 ), // up right arrow
	M( 0, 8 ), // Z-with-hat
	M( 0, 8 ), // down left arrow
	M( 0, 8 ), // down right arrow 0x90
	M( 0, 8 ), // ´
	M( 0, 8 ), // ´ ???
	M( 0, 8 ), // "
	M( 0, 8 ), // " ???
	M( 0, 8 ), // dot
	M( 0, 8 ), // - en-dash
	M( 0, 8 ), // - em-dash
	M( 0, 8 ), // ~
	M( 0, 8 ), // TM
	M( 0, 7 ), // s-with-hat
	M( 1, 7 ), // > quotation
	M( 0, 7 ), // oe
	M( 1, 7 ), // down arrow
	M( 0, 7 ), // z-with-hat
	M( 0, 8 ), // Y-with-dots
	M( 0, 3 ), // nbsp 0xA0
	M( 0, 8 ), // i... or upside-down !
	M( 0, 8 ), // c (cent sign)
	M( 0, 8 ), // £ (pound sign)
	M( 0, 8 ), // ¤ (currency symbol)
	M( 0, 8 ), // Yen
	M( 0, 8 ), // | with cut
	M( 0, 8 ), // §
	M( 0, 8 ), // ¨ for reals this time ?
	M( 0, 8 ), // (C)
	M( 0, 8 ), // some blob
	M( 0, 8 ), // <<
	M( 0, 8 ), // -| some thing 
	M( 0, 8 ), // --
	M( 0, 8 ), // (R)
	M( 0, 8 ), // up line
	M( 0, 8 ), // degree sign 0xB0
	M( 0, 8 ), // +-
	M( 0, 8 ), // ^2
	M( 0, 8 ), // ^3
	M( 2, 4 ), // ´
	M( 1, 7 ), // µ
	M( 0, 8 ), // paragraph
	M( 0, 8 ), // mid dot
	M( 0, 8 ), // cedilla
	M( 0, 8 ), // ^1
	M( 0, 8 ), // up-O-with-supposed-line
	M( 0, 8 ), // >>
	M( 0, 8 ), // 1/4
	M( 0, 8 ), // 1/2
	M( 0, 8 ), // 3/4
	M( 0, 8 ), // upside down ?
	M( 0, 8 ), // A with ` 0xC0
	M( 0, 8 ), // A with ´
	M( 0, 8 ), // A with ^
	M( 0, 8 ), // A with ~
	M( 0, 8 ), // Ä
	M( 0, 8 ), // Å
	M( 0, 8 ), // AE
	M( 0, 8 ), // C with cedilla
	M( 0, 8 ), // E with `
	M( 0, 8 ), // E with ´
	M( 0, 8 ), // E with ^
	M( 0, 8 ), // Ë
	M( 0, 8 ), // I with `
	M( 0, 8 ), // I with ´
	M( 0, 8 ), // I with ^
	M( 0, 8 ), // Ï
	M( 0, 8 ), // D with - inside 0xD0
	M( 0, 8 ), // Ñ
	M( 0, 8 ), // O with `
	M( 0, 8 ), // O with ´
	M( 0, 8 ), // O with ^
	M( 0, 8 ), // O with ~
	M( 0, 8 ), // Ö
	M( 0, 8 ), // X -like
	M( 0, 8 ), // O with /
	M( 0, 8 ), // U with `
	M( 0, 8 ), // U with ´
	M( 0, 8 ), // U with ^
	M( 0, 8 ), // Ü
	M( 0, 8 ), // Ý
	M( 0, 8 ), // B-like/D-like
	M( 0, 8 ), // Ss
	M( 0, 7 ), // à 0xE0
	M( 0, 7 ), // á
	M( 0, 7 ), // â
	M( 0, 7 ), // ã
	M( 0, 7 ), // ä
	M( 0, 7 ), // å
	M( 0, 7 ), // ae
	M( 0, 7 ), // c with cedilla
	M( 0, 7 ), // è
	M( 0, 7 ), // é
	M( 0, 7 ), // ê
	M( 0, 7 ), // ë
	M( 0, 7 ), // ì
	M( 0, 7 ), // í
	M( 0, 7 ), // î
	M( 0, 7 ), // ï
	M( 0, 7 ), // d x (small d with line) 0xF0
	M( 0, 7 ), // ñ
	M( 0, 7 ), // ò
	M( 0, 7 ), // ó
	M( 0, 7 ), // ô
	M( 0, 7 ), // õ
	M( 0, 7 ), // ö
	M( 1, 7 ), // division -- ..
	M( 0, 7 ), // o with /
	M( 0, 7 ), // ù
	M( 0, 7 ), // ú
	M( 0, 7 ), // û
	M( 0, 7 ), // ü
	M( 0, 7 ), // ý
	M( 0, 7 ), // thorn (B-like/D-like small)
	M( 0, 7 )  // ÿ
};
