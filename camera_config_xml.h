#ifndef _CAMERA_CONFIG_XML_H_
#define _CAMERA_CONFIG_XML_H_

#define DEFAULT_CAMERA_CONFIG_XML \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"  		\
"<epcs_protocol xmlns=\"http://vdcs.bitcom.com\">	\n" \
"	<camera_config>                                 \n" \
"		<exp_mode>                                  \n" \
"			<flag>1</flag>                          \n" \
"		</exp_mode>                                 \n" \
"		<exp_manu>                                  \n" \
"			<exp>2</exp>                            \n" \
"			<gain>2</gain>                          \n" \
"		</exp_manu>                                 \n" \
"		<exp_auto>                                  \n" \
"			<light_dt_mode>1</light_dt_mode>        \n" \
"			<exp_max>400</exp_max>                  \n" \
"			<gain_max>40</gain_max>                 \n" \
"			<exp_mid>200</exp_mid>                  \n" \
"			<gain_mid>20</gain_mid>                 \n" \
"			<exp_min>10</exp_min>                   \n" \
"			<gain_min>1</gain_min>                  \n" \
"		</exp_auto>                                 \n" \
"		<exp_wnd>                                   \n" \
"			<line1>0</line1>                        \n" \
"			<line2>0</line2>                        \n" \
"			<line3>0</line3>                        \n" \
"			<line4>0</line4>                        \n" \
"			<line5>255</line5>                      \n" \
"			<line6>255</line6>                      \n" \
"			<line7>255</line7>                      \n" \
"			<line8>255</line8>                      \n" \
"		</exp_wnd>                                  \n" \
"		<awb_mode>                                  \n" \
"			<flag>1</flag>                          \n" \
"		</awb_mode>                                 \n" \
"		<awb_manu>                                  \n" \
"			<gain_r>256</gain_r>                    \n" \
"			<gain_g>256</gain_g>                    \n" \
"			<gain_b>256</gain_b>                    \n" \
"		</awb_manu>                                 \n" \
"		<color_param>                               \n" \
"			<contrast>200</contrast>                \n" \
"			<luma>130</luma>                        \n" \
"			<saturation>34</saturation>             \n" \
"		</color_param>                              \n" \
"		<syn_info>                                  \n" \
"			<is_syn_open>1</is_syn_open>            \n" \
"			<phase>0</phase>                        \n" \
"		</syn_info>                                 \n" \
"		<lamp_info>                                 \n" \
"			<nMode id=\"1\">0</nMode>               \n" \
"			<nLampType id=\"1\">0</nLampType>       \n" \
"			<nMode id=\"2\">0</nMode>               \n" \
"			<nLampType id=\"2\">0</nLampType>       \n" \
"			<nMode id=\"3\">0</nMode>               \n" \
"			<nLampType id=\"3\">0</nLampType>       \n" \
"			<nMode id=\"4\">0</nMode>               \n" \
"			<nLampType id=\"4\">0</nLampType>       \n" \
"		</lamp_info>                                \n" \
"	</camera_config>                                \n" \
"</epcs_protocol>                                   \n"

#endif
