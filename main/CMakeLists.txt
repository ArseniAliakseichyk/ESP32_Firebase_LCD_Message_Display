idf_component_register(SRCS "main.c"
    SRCS 
        "main.c"
        "lcd/lcd.c"
        "rest_client/rest_client.c"
    INCLUDE_DIRS 
        "."
        "./lcd"
        "./rest_client"
)
