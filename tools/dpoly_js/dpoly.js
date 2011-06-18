var dpoly = {
	m_playing : false,
	m_yield : 0,
	m_clear : false,
	m_buf : null,
	m_palette : new Array( 32 ),
	m_scale : 2,

	init : function( canvas ) {
		this.m_canvas = document.getElementById( canvas );
		this.m_cmd = this.loadBinary( 'intro.cmd' );
		this.m_pol = this.loadBinary( 'intro.pol' );
		this.m_pos = 2;
	},

	start : function( ) {
		this.m_playing = true;
		this.setDefaultPalette( );
		setInterval( function( ) { dpoly.doTick( ) }, 20 );
	},

	readByte : function( buf, pos ) {
		var value = buf[ pos ];
		return value;
	},

	readWord : function( buf, pos ) {
		var value = buf[ pos ] * 256 + buf[ pos + 1 ];
		return value;
	},

	readNextByte : function( ) {
		var value = this.readByte( this.m_cmd, this.m_pos );
		this.m_pos += 1;
		return value;
	},

	readNextWord : function( ) {
		var value = this.readWord( this.m_cmd, this.m_pos );
		this.m_pos += 2;
		return value;
	},

	toSignedByte : function( value ) {
		if (value & 0x80) {
			return value - 0x100;
		}
		return value;
	},

	toSignedWord : function( value ) {
		if (value & 0x8000) {
			return value - 0x10000;
		}
		return value;
	},

	doTick : function( ) {
		if ( !this.m_playing ) {
			return;
		}
		if ( this.m_yield != 0 ) {
			this.m_yield -= 1;
			return;
		}
		this.flipScreen( );
		while ( this.m_yield == 0 ) {
			var opcode = this.readNextByte( );
			window.console.log('current opcode=' + opcode + ' pos=' + this.m_pos);
			if (opcode & 0x80) {
				this.m_playing = false;
				break;
			}
			switch (opcode >> 2) {
			case 0:
			case 5:
			case 9:
				this.updateScreen( );
				break;
			case 1:
				this.m_clear = this.readNextByte( );
				this.clearScreen( );
				break;
			case 2:
				this.m_yield = this.readNextByte( ) * 4;
				break;
			case 3:
				var shape = this.readNextWord( );
				var x = 0;
				var y = 0;
				if (shape & 0x8000) {
					shape = shape & 0x7FFF;
					x = this.toSignedWord( this.readNextWord( ) );
					y = this.toSignedWord( this.readNextWord( ) );
				}
				this.drawShape( shape, x, y );
				break;
			case 4:
				var src = this.readNextByte( );
				var dst = this.readNextByte( );
				this.setPalette( src, dst );
				break;
			case 10:
				var shape = this.readNextWord( );
				var x = 0;
				var y = 0;
				if (shape & 0x8000) {
					shape = shape & 0x7FFF;
					x = this.toSignedWord( this.readNextWord( ) );
					y = this.toSignedWord( this.readNextWord( ) );
				}
				var z = 512 + this.readNextWord( );
				var ix = this.readNextByte( );
				var iy = this.readNextByte( );
				this.drawShapeScale( shape, x, y, z, ix, iy );
				break;
			case 11:
				var shape = this.readNextWord( );
				var x = 0;
				var y = 0;
				if (shape & 0x8000) {
					shape = shape & 0x7FFF;
					x = this.toSignedWord( this.readNextWord( ) );
					y = this.toSignedWord( this.readNextWord( ) );
				}
				var z = 512;
				if (shape & 0x4000) {
					shape = shape & 0x3FFF;
					z += this.readNextWord( );
				}
				var ix = this.readNextByte( );
				var iy = this.readNextByte( );
				var r1 = this.readNextWord( );
				var r2 = 90;
				if (shape & 0x2000) {
					shape = shape & 0x1FFF;
					r2 = this.readNextWord( );
				}
				var r3 = 180;
				if (shape & 0x1000) {
					shape = shape & 0xFFF;
					r3 = this.readNextWord( );
				}
				this.drawShapeScaleRotate( shape, x, y, z, ix, iy, r1, r2, r3 );
				break;
			case 12:
				// memcpy page1 page0
				this.m_yield = 10;
				break;
			default:
				throw 'Invalid opcode ' + opcode;
//				this.m_playing = false;
				break;
			}
		}
	},

	updateScreen : function( ) {
		this.m_yield = 5;
//		this.flipScreen( );
		// draw page0
	},

	clearScreen : function( ) {
		if (this.m_clear) {
			this.flipScreen( );
		}
	},

	drawShape : function( num, x, y ) {
		var offset = this.readWord( this.m_pol, 2 );
		var shapeOffset = this.readWord( this.m_pol, offset + num * 2 );

		offset = this.readWord( this.m_pol, 14 );
		offset += shapeOffset;

		var count = this.readWord( this.m_pol, offset );
		offset += 2;

		var dx, dy;
		for (var i = 0; i < count; ++i) {
			var verticesOffset = this.readWord( this.m_pol, offset );
			offset += 2;
			if (verticesOffset & 0x8000) {
				dx = this.toSignedWord( this.readWord( this.m_pol, offset ) );
				offset += 2;
				dy = this.toSignedWord( this.readWord( this.m_pol, offset ) );
				offset += 2;
			} else {
				dx = 0;
				dy = 0;
			}
			var alpha = (verticesOffset & 0x4000) != 0;
			var color = this.readByte( this.m_pol, offset );
			offset++;

			if (this.m_clear == 0) {
				color += 16;
			}

			this.drawPrimitive( x + dx, y + dy, verticesOffset & 0x3FFF, color, alpha );
		}
		if (this.m_clear) {
			this.copyScreen( );
		}
	},

	drawShapeScale : function( num, x, y, z, ix, iy ) {
		// TODO:
		// +1 sur le verticesDataTable
	},

	drawShapeScaleRotate : function( num, x, y, z, ix, iy, r1, r2, r3 ) {
	},

	setDefaultPalette : function( ) {
		for ( var i = 0; i < 16; i++ ) {
			var color = '#' + i.toString( 16 ) + i.toString( 16 ) + i.toString( 16 );
			this.m_palette[ 16 + i ] = this.m_palette[ i ] = color;
		}
	},

	setPalette : function( src, dst ) {
		var offset = this.readWord( this.m_pol, 6 );
		offset += src * 32;

		var palOffset = 0;
		if ( dst == 0 ) {
			palOffset = 16;
		}

		for ( var i = 0; i < 16; i++ ) {
			var color = this.readWord( this.m_pol, offset );
			offset += 2;

			var r = (color >> 8) & 15;
			var g = (color >> 4) & 15;
			var b = color & 15;
			this.m_palette[ palOffset ] = '#' + r.toString( 16 ) + g.toString( 16 ) + b.toString( 16 );
			++palOffset;
		}
	},

	copyScreen : function( ) {
		var context = this.m_canvas.getContext( '2d' );
		this.m_buf = context.getImageData( 0, 0, this.m_canvas.width, this.m_canvas.height );
	},

	flipScreen : function( ) {
		var context = this.m_canvas.getContext( '2d' );
		if ( this.m_clear ) {
			context.fillStyle = '#000';
			context.fillRect( 0, 0, this.m_canvas.width, this.m_canvas.height );
		} else {
			if ( this.m_buf ) {
				context.putImageData( this.m_buf, 0, 0 );
			}
		}
	},

	drawPrimitive : function( x, y, num, color, alpha ) {
		var offset = this.readWord( this.m_pol, 10 );
		var verticesOffset = this.readWord( this.m_pol, offset + num * 2 );

		offset = this.readWord( this.m_pol, 18 );
		offset += verticesOffset;

		var count = this.readByte( this.m_pol, offset );
		offset++;

		x += this.toSignedWord( this.readWord( this.m_pol, offset ) );
		offset += 2;
		y += this.toSignedWord( this.readWord( this.m_pol, offset ) );
		offset += 2;

		var scale = this.m_scale;

		var context = this.m_canvas.getContext( '2d' );
		context.fillStyle = this.m_palette[ color ];
		context.save( );
		context.scale( scale, scale );
		if (count & 0x80) {
			context.translate( x, y );
			var rx = this.toSignedWord( this.readWord( this.m_pol, offset ) );
			offset += 2;
			var ry = this.toSignedWord( this.readWord( this.m_pol, offset ) );
			offset += 2;
			context.scale( rx, ry / rx );
			context.beginPath( );
			context.arc( 0, 0, rx, 0, 2 * Math.PI, false );
			context.closePath( );
			context.fill( );
		} else if (count == 0) {
			context.fillRect( x, y, scale, scale );
		} else {
			--count;
			context.beginPath( );
			context.moveTo( x, y );
			while (count >= 0) {
				var dx = this.toSignedByte( this.readByte( this.m_pol, offset ) );
				offset++;
				var dy = this.toSignedByte( this.readByte( this.m_pol, offset ) );
				offset++;
				x += dx;
				y += dy;
				context.lineTo( x, y );
				--count;
			}
			context.closePath( );
			context.fill( );
		}
		context.restore( );
	},

	loadBinary : function( name ) {
		var req = new XMLHttpRequest( );
		req.open( 'GET', name, false );
		if (req.hasOwnProperty('responseType')) {
			req.responseType = 'arraybuffer';
		} else {
			req.overrideMimeType('text/plain; charset=x-user-defined');
		}
		req.send(null);
		var ret;
		if (req.mozResponseArrayBuffer) {
			ret = req.mozResponseArrayBuffer;
		} else if (req.hasOwnProperty('responseType')) {
			ret = req.response;
		} else {
			ret = req.responseText;
		}
		var buf = new Uint8Array(ret, 0, ret.byteLength);
		return buf;
	}
}
