PWM control to plug into the level control cluster:
----------------------------------------------------
This plugin is designed to work with the level control cluster.  It implements the PWM drive level for an LED light bulb on PA6.  

Instructions:
1) Drop these files into your 5.1.0 release under the directory app/framework/plugins/pwm-control\
2) create a level control light project.  Include the level control plugin.
3) include the PWM control plugin into your project.
4) make sure to connect the PWM drive circuit for your LED circuit onto GPIO PA6, and configure PA6 as an alternate output PP.
5) after generating, you will need to delete buzzer.c from the IAR project file.


