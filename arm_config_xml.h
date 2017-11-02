#ifndef _ARM_CONFIG_XML_H_
#define _ARM_CONFIG_XML_H_

#define DEFAULT_ARM_CONFIG_XML \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n" \
"<epcs_protocol xmlns=\"http://vdcs.ipnc.com\">    \n" \
"	<arm_config>    \n" \
"		<basic_param>    \n" \
"			<monitor_type>1</monitor_type>    \n" \
"			<spot_id>370203456789</spot_id>    \n" \
"			<device_id/>    \n" \
"			<road_id>4321</road_id>    \n" \
"			<spot>江西路</spot>    \n" \
"			<direction>1</direction>    \n" \
"			<ntp_config_param>    \n" \
"				<useNTP>0</useNTP>    \n" \
"				<NTP_server_ip id=\"1\">0</NTP_server_ip>    \n" \
"				<NTP_server_ip id=\"2\">0</NTP_server_ip>    \n" \
"				<NTP_server_ip id=\"3\">0</NTP_server_ip>    \n" \
"				<NTP_server_ip id=\"4\">0</NTP_server_ip>    \n" \
"				<NTP_distance>0</NTP_distance>    \n" \
"			</ntp_config_param>    \n" \
"			<exp_type>1</exp_type>    \n" \
"			<exp_device_id>123456789012</exp_device_id>    \n" \
"			<collect_actor_size>12</collect_actor_size>    \n" \
"			<log_level>0</log_level>    \n" \
"			<data_save>    \n" \
"				<disk_wri_data>    \n" \
"					<remain_disk>10</remain_disk>    \n" \
"					<illegal_picture>0</illegal_picture>    \n" \
"					<vehicle>0</vehicle>    \n" \
"					<event_picture>0</event_picture>    \n" \
"					<illegal_video>0</illegal_video>    \n" \
"					<event_video>0</event_video>    \n" \
"					<flow_statistics>0</flow_statistics>    \n" \
"				</disk_wri_data>    \n" \
"				<ftp_data_config>    \n" \
"					<illegal_picture>1</illegal_picture>    \n" \
"					<vehicle>1</vehicle>    \n" \
"					<event_picture>1</event_picture>    \n" \
"					<illegal_video>1</illegal_video>    \n" \
"					<event_video>1</event_video>    \n" \
"					<flow_statistics>0</flow_statistics>    \n" \
"				</ftp_data_config>    \n" \
"				<resume_upload_data>    \n" \
"					<is_resume_illegal>0</is_resume_illegal>    \n" \
"					<is_resume_event>0</is_resume_event>    \n" \
"					<is_resume_passcar>0</is_resume_passcar>    \n" \
"					<is_resume_statistics>0</is_resume_statistics>    \n" \
"				</resume_upload_data>    \n" \
"			</data_save>    \n" \
"			<h264_record>1</h264_record>    \n" \
"			<ftp_param_pass_car>    \n" \
"				<user>anonymous</user>    \n" \
"				<passwd>anonymous</passwd>    \n" \
"				<ip id=\"1\">172</ip>    \n" \
"				<ip id=\"2\">16</ip>    \n" \
"				<ip id=\"3\">5</ip>    \n" \
"				<ip id=\"4\">158</ip>    \n" \
"				<port>21</port>    \n" \
"				<allow_anonymous>1</allow_anonymous>    \n" \
"			</ftp_param_pass_car>    \n" \
"			<ftp_param_illegal>    \n" \
"				<user>anonymous</user>    \n" \
"				<passwd>anonymous</passwd>    \n" \
"				<ip id=\"1\">172</ip>    \n" \
"				<ip id=\"2\">16</ip>    \n" \
"				<ip id=\"3\">5</ip>    \n" \
"				<ip id=\"4\">158</ip>    \n" \
"				<port>21</port>    \n" \
"				<allow_anonymous>1</allow_anonymous>    \n" \
"			</ftp_param_illegal>    \n" \
"			<ftp_param_h264>    \n" \
"				<user>anonymous</user>    \n" \
"				<passwd>anonymous</passwd>    \n" \
"				<ip id=\"1\">172</ip>    \n" \
"				<ip id=\"2\">16</ip>    \n" \
"				<ip id=\"3\">5</ip>    \n" \
"				<ip id=\"4\">158</ip>    \n" \
"				<port>21</port>    \n" \
"				<allow_anonymous>1</allow_anonymous>    \n" \
"			</ftp_param_h264>    \n" \
"		</basic_param>    \n" \
"		<interface_param>    \n" \
"			<serial id=\"1\">    \n" \
"				<dev_type>0</dev_type>    \n" \
"				<bps>9600</bps>    \n" \
"				<check>0</check>    \n" \
"				<data>8</data>    \n" \
"				<stop>1</stop>    \n" \
"			</serial>    \n" \
"			<serial id=\"2\">    \n" \
"				<dev_type>0</dev_type>    \n" \
"				<bps>4800</bps>    \n" \
"				<check>0</check>    \n" \
"				<data>8</data>    \n" \
"				<stop>1</stop>    \n" \
"			</serial>    \n" \
"			<serial id=\"3\">    \n" \
"				<dev_type>0</dev_type>    \n" \
"				<bps>2400</bps>    \n" \
"				<check>0</check>    \n" \
"				<data>8</data>    \n" \
"				<stop>1</stop>    \n" \
"			</serial>    \n" \
"			<io_input_params id=\"1\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_input_params id=\"2\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_input_params id=\"3\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_input_params id=\"4\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_input_params id=\"5\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_input_params id=\"6\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_input_params id=\"7\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_input_params id=\"8\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"				<io_drt>0</io_drt>    \n" \
"			</io_input_params>    \n" \
"			<io_output_params id=\"1\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"			</io_output_params>    \n" \
"			<io_output_params id=\"2\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"			</io_output_params>    \n" \
"			<io_output_params id=\"3\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"			</io_output_params>    \n" \
"			<io_output_params id=\"4\">    \n" \
"				<trigger_type>0</trigger_type>    \n" \
"				<mode>0</mode>    \n" \
"			</io_output_params>    \n" \
"		</interface_param>    \n" \
"		<h264_config>    \n" \
"			<h264_channel id=\"1\">    \n" \
"				<h264_on>1</h264_on>    \n" \
"				<cast>1</cast>    \n" \
"				<ip id=\"1\">192</ip>    \n" \
"				<ip id=\"2\">168</ip>    \n" \
"				<ip id=\"3\">1</ip>    \n" \
"				<ip id=\"4\">158</ip>    \n" \
"				<port>23456</port>    \n" \
"				<fps>12</fps>    \n" \
"				<rate>6000</rate>    \n" \
"				<width>3264</width>    \n" \
"				<height>2464</height>    \n" \
"				<osd_info>    \n" \
"					<color>    \n" \
"						<r>255</r>    \n" \
"						<g>255</g>    \n" \
"						<b>255</b>    \n" \
"					</color>    \n" \
"					<osd_item id=\"1\">    \n" \
"						<switch_on>1</switch_on>    \n" \
"						<is_time>1</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>100</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"2\">    \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>1</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>100</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"3\">    \n" \
"						<switch_on>1</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>200</y>    \n" \
"						<content>你好</content>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"4\">    \n" \
"						<switch_on>1</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>100</y>    \n" \
"						<content>欢迎</content>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"5\">    \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>0</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"6\">    \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>0</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"7\">    \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>0</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"8\">    \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>0</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"				</osd_info>    \n" \
"			</h264_channel>    \n" \
"			<h264_channel id=\"2\">    \n" \
"				<h264_on>1</h264_on>    \n" \
"				<cast>1</cast>    \n" \
"				<ip id=\"1\">224</ip>    \n" \
"				<ip id=\"2\">0</ip>    \n" \
"				<ip id=\"3\">1</ip>    \n" \
"				<ip id=\"4\">158</ip>    \n" \
"				<port>45678</port>    \n" \
"				<fps>12</fps>    \n" \
"				<rate>1000</rate>    \n" \
"				<width>720</width>    \n" \
"				<height>180</height>    \n" \
"				<osd_info>    \n" \
"					<color>    \n" \
"						<r>255</r>    \n" \
"						<g>255</g>    \n" \
"						<b>255</b>    \n" \
"					</color>    \n" \
"					<osd_item id=\"1\">    \n" \
"						<switch_on>1</switch_on>    \n" \
"						<is_time>1</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>100</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"2\">    \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>1</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>100</y>    \n" \
"						<content/>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"3\">    \n" \
"						<switch_on>1</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>200</y>    \n" \
"						<content>你好</content>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"4\">    \n" \
"						<switch_on>1</switch_on>    \n" \
"						<is_time>0</is_time>    \n" \
"						<x>0</x>    \n" \
"						<y>100</y>    \n" \
"						<content>欢迎</content>    \n" \
"					</osd_item>    \n" \
"					<osd_item id=\"5\">    \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>        \n" \
"						<x>0</x>                    \n" \
"						<y>0</y>                    \n" \
"						<content/>                  \n" \
"					</osd_item>                     \n" \
"					<osd_item id=\"6\">             \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>        \n" \
"						<x>0</x>                    \n" \
"						<y>0</y>                    \n" \
"						<content/>                  \n" \
"					</osd_item>                     \n" \
"					<osd_item id=\"7\">             \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>        \n" \
"						<x>0</x>                    \n" \
"						<y>0</y>                    \n" \
"						<content/>                  \n" \
"					</osd_item>                     \n" \
"					<osd_item id=\"8\">             \n" \
"						<switch_on>0</switch_on>    \n" \
"						<is_time>0</is_time>        \n" \
"						<x>0</x>                    \n" \
"						<y>0</y>                    \n" \
"						<content/>                  \n" \
"					</osd_item>                     \n" \
"				</osd_info>                         \n" \
"			</h264_channel>                         \n" \
"		</h264_config>                              \n" \
"		<illegal_code_info id=\"1\">                \n" \
"			<illeagal_type>8192</illeagal_type>     \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"2\">                \n" \
"			<illeagal_type>4096</illeagal_type>     \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"3\">                \n" \
"			<illeagal_type>2048</illeagal_type>     \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"4\">                \n" \
"			<illeagal_type>1024</illeagal_type>     \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"5\">                \n" \
"			<illeagal_type>512</illeagal_type>      \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"6\">                \n" \
"			<illeagal_type>256</illeagal_type>      \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"7\">                \n" \
"			<illeagal_type>128</illeagal_type>      \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"8\">                \n" \
"			<illeagal_type>64</illeagal_type>       \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"9\">                \n" \
"			<illeagal_type>32</illeagal_type>       \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"10\">               \n" \
"			<illeagal_type>16</illeagal_type>       \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"11\">               \n" \
"			<illeagal_type>8</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"12\">               \n" \
"			<illeagal_type>4</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"13\">               \n" \
"			<illeagal_type>2</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"14\">               \n" \
"			<illeagal_type>1</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"15\">               \n" \
"			<illeagal_type>0</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"16\">               \n" \
"			<illeagal_type>0</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"17\">               \n" \
"			<illeagal_type>0</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"18\">               \n" \
"			<illeagal_type>0</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"19\">               \n" \
"			<illeagal_type>0</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"		<illegal_code_info id=\"20\">               \n" \
"			<illeagal_type>0</illeagal_type>        \n" \
"			<illeagal_num>0</illeagal_num>          \n" \
"		</illegal_code_info>                        \n" \
"	</arm_config>                                   \n" \
"</epcs_protocol>                                   \n"

#endif
