EXTRA_DIST = asciidoc.conf
CLEANFILES =

MAN3 = \
        tta_close.3 \
        tta_connect.3 \
        tta_flush.3 \
        tta_free.3 \
        tta_get_byte_from_bits.3 \
        tta_get_byte_timeout.3 \
        tta_get_float.3 \
        tta_get_float_dcba.3 \
        tta_get_header_length.3 \
        tta_get_response_timeout.3 \
        tta_get_socket.3 \
        tta_mapping_free.3 \
        tta_mapping_new.3 \
        tta_mask_write_register.3 \
        tta_new_rtu.3 \
        tta_new_tcp_pi.3 \
        tta_new_tcp.3 \
        tta_read_bits.3 \
        tta_read_input_bits.3 \
        tta_read_input_registers.3 \
        tta_read_registers.3 \
        tta_receive_confirmation.3 \
        tta_receive_from.3 \
        tta_receive.3 \
        tta_reply_exception.3 \
        tta_reply.3 \
        tta_report_slave_id.3 \
        tta_rtu_get_serial_mode.3 \
        tta_rtu_set_serial_mode.3 \
        tta_rtu_get_rts.3 \
        tta_rtu_set_rts.3 \
        tta_send_raw_request.3 \
        tta_set_bits_from_bytes.3 \
        tta_set_bits_from_byte.3 \
        tta_set_byte_timeout.3 \
        tta_set_debug.3 \
        tta_set_error_recovery.3 \
        tta_set_float.3 \
        tta_set_float_dcba.3 \
        tta_set_response_timeout.3 \
        tta_set_socket.3 \
        tta_strerror.3 \
        tta_tcp_listen.3 \
        tta_write_and_read_registers.3 \
        tta_write_bits.3 \
        tta_write_bit.3 \
        tta_write_registers.3 \
        tta_write_register.3
MAN7 = libtta.7

MAN_DOC = $(MAN3) $(MAN7)

MAN_TXT = $(MAN3:%.3=%.txt)
MAN_TXT += $(MAN7:%.7=%.txt)
MAN_HTML = $(MAN_TXT:%.txt=%.html)

if INSTALL_MAN
dist_man_MANS = $(MAN_DOC)
doc: $(MAN_DOC)
endif

EXTRA_DIST += $(MAN_TXT)
if BUILD_DOC
EXTRA_DIST += $(MAN_HTML)
html: $(MAN_HTML)
endif

MAINTAINERCLEANFILES = $(MAN_DOC) $(MAN_HTML)

dist-hook: $(MAN_DOC) $(MAN_HTML)

if BUILD_DOC
SUFFIXES=.html .txt .xml .1 .3 .7

.txt.html:
	asciidoc -d manpage -b xhtml11 -f asciidoc.conf \
		-alibtta_version=@LIBMODBUS_VERSION@ $<
.txt.xml:
	asciidoc -d manpage -b docbook -f asciidoc.conf \
		-alibtta_version=@LIBMODBUS_VERSION@ $<
.xml.1:
	xmlto man $<
.xml.3:
	xmlto man $<
.xml.7:
	xmlto man $<

CLEANFILES += *.1 *.3 *.7 *.html
endif
