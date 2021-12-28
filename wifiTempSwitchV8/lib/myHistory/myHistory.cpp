#include <Arduino.h>
#include <string.h>

// custom libs
#include "myHistory.h"
#include <myTemp.h>
#include <myTime.h>
#include <myWifi.h>

extern bool debug;

// local variables ======================================
float air_temp_5m[144];
float air_humidity_5m[144];
char time_5m[144][6];
uint8_t index_5m = 0;
bool array_5m_filled = false;

// iterate through array and return comma delimited string
String return_int_array_history(int array[], uint8_t array_length, uint8_t start_index)
{
    String output = "";

    if (start_index == 0)
    {
        for (int i = 0; i < (array_length - 1); i++) // iterate over array length
        {
            output += String(array[i]) + ", ";
        }
        output += String(array[array_length - 1]);
    }
    else
    {
        for (int i = start_index; i < array_length; i++) // start at start_index and iterate to end
        {
            output += String(array[i]) + ", ";
        }
        for (int i = 0; i < (start_index - 1); i++) // start at 0 and go to start_index - 2
        {
            output += String(array[i]) + ", ";
        }
        output += String(array[start_index - 1]) ; // end array at start_index - 1
    }

    return output;
}

// iterate through array and return comma delimited string
String return_float_array_history(float array[], uint8_t array_length, uint8_t start_index)
{
    String output = "";

    if (start_index == 0)
    {
        for (int i = 0; i < (array_length - 1); i++) // iterate over array length
        {
            output += String(array[i]) + ", ";
        }
        output += String(array[array_length - 1]);
    }
    else
    {
        for (int i = start_index; i < array_length; i++) // start at start_index and iterate to end
        {
            output += String(array[i]) + ", ";
        }
        for (int i = 0; i < (start_index - 1); i++) // start at 0 and go to start_index - 2
        {
            output += String(array[i]) + ", ";
        }
        output += String(array[start_index - 1]) ; // end array at start_index - 1
    }

    return output;
}

// iterate through array and return comma delimited string
String return_char_array_history(char array[][6], uint8_t array_length, uint8_t start_index)
{
    String output = "";

    if (start_index == 0)
    {
        for (uint8_t i = 0; i < (array_length - 1); i++) // iterate over array length
        {
            output += "\"" + String(array[i]) + "\", ";
        }
        output += "\"" + String(array[array_length - 1]) + "\"";
    }
    else
    {
        for (uint8_t i = start_index; i < array_length; i++) // start at start_index and iterate to end
        {
            output += "\"" + String(array[i]) + "\", ";
        }
        for (uint8_t i = 0; i < (start_index - 1); i++) // start at 0 and go to start_index - 2
        {
            output += "\"" + String(array[i]) + "\", ";
        }
        output += "\"" + String(array[start_index - 1]) + "\""; // end array at start_index - 1
    }

    return output;
}

// return history html
String return_history_html(){
    uint8_t start_index = 0;
    if(array_5m_filled == true) start_index = index_5m;
    String time_data_array = "";

    if (return_time_client_status() == true  && myWifi_return_connection_status() == 3)
    {
        time_data_array = return_char_array_history(time_5m, index_5m, start_index);
    }
    else // no internet connection, so fill time array with numbers
    {        
        if (array_5m_filled == true)
        {
            for (uint8_t i = 0; i < 143; i++)
            {
                time_data_array += String(i)+",";
            }
            time_data_array += "143";
        }
        else
        {
            for (uint8_t i = 0; i < (index_5m - 1); i++)
            {
                time_data_array += String(i)+",";
            }
            time_data_array += String(index_5m - 1);
        }
    }

    return " <!DOCTYPE html>"
    "<html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><meta http-equiv=\"refresh\" content=\"180; url='/history'\"/></head>"
    "<style>"
    "body {"
    "position: relative;"
    "top: 0;"
    "left: 0;"
    "color: white;"
    "background-color: transparent;"
    "height: calc(100vh - 10px);"
    "width: calc(100vw - 10px);"
    "border: #ffffff solid 5px;"
    "border-radius: 20px;"
    "margin: 0;"
    "padding: 0;"
    "z-index: 0;"
    "}"
    ".flexContainer {"
    "background-image: linear-gradient(180deg, #18453B, black);"
    "z-index: 1;"
    "position: relative;"
    "margin: 0;"
    "padding: 0;"
    "width: 100%;"
    "height: 100%;"
    "display: flex;"
    "text-align: center;"
    "align-items: center;"
    "align-content: center;"
    "flex-wrap: nowrap;"
    "flex-direction: column;"
    "overflow-y: scroll;"
    "font-weight: bold;"
    "}"
    ".title {"
    "padding: 10px;"
    "font-size: x-large;"
    "}"
    ".subTitle {"
    "padding: 5px;"
    "font-size: large;"
    "}"
    ".subFlexBlock {"
    "z-index: 2;"
    "font-size: large;"
    "position: relative;"
    "left: 0;"
    "margin: 0;"
    "margin-bottom: 10px;"
    "padding: 10px;"
    "width: 100%;"
    "max-width: 300px;"
    "height: auto;"
    "display: flex;"
    "flex-direction: column;"
    "text-align: center;"
    "align-items: center;"
    "align-content: center;"
    "}"
    ".greenBorder {"
    "background-color: black;"
    "color: white;"
    "width: 90%;"
    "border: #18453B solid 6px;"
    "border-radius: 20px;"
    "}"
    ".whiteBorder {"
    "background-color: #818181;"
    "color: #18453B;"
    "width: 80%;"
    "border: #ffffff solid 6px;"
    "border-radius: 20px;"
    "}"
    ".blackBorder {"
    "background-color: #18453B;"
    "color: white;"
    "width: 85%;"
    "border: #000000 solid 6px;"
    "border-radius: 20px;"
    "}"
    ".textBlocks {"
    "font-size: large;"
    "text-align: center;"
    "align-content: center;"
    "align-items: center;"
    "padding: 2px;"
    "margin: 0;"
    "}"
    "button {"
    "margin: 0;"
    "font-size: large;"
    "padding: 5px 10px 5px 10px;"
    "color: #b4b4b4;"
    "background-color: black;"
    "border-radius: 10px;"
    "border: #818181 solid 4px;" // grey
    "text-decoration: underline #b4b4b4;"
    "}"
    ".greyText{"
    "color: #b4b4b4;"
    "}"
    "</style>"
    "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js\"></script>"
    // ----------
    "<body>"
    "<div class='flexContainer'>"
    "</br>"
    "<p class='textBlocks'><a href=\"/\"><button style='display: block;'>Home</button></a></p>"
    "<p>"+ String(time_5m[index_5m][0]) + "</p>"
    // --- air temp 
    "<div style=\"width: 100%; max-width:600px;padding:10px;background-color:white;\">"
    "<canvas id=\"myChart0\" style=\"width:100%;max-width:600px\"></canvas>"
    "</div>"
    "<script>"
    "new Chart(\"myChart0\", {"
    "  type: \"line\","
    "  data: {"
    "  labels: ["  + time_data_array + "],"
    "datasets: [{ "
    "data: [" + return_float_array_history(air_temp_5m, index_5m, start_index) + "],"
    "borderColor: \"red\","
    "fill: false"
    "  }]"
    " },"
    "options: {"
    "title: {  display: true, text: 'Air Temp over Time' },"
    "legend: {display: false},"
    " responsive: true,"
    "scales: {"
    "xAxes: [ {"
    "display: true,"
    "scaleLabel: {"
    "display: true,"
    "labelString: 'Time'"
    "},"
    "ticks: {"
    "major: {"
    "fontStyle: 'bold',"
    "fontColor: '#FF0000'"
    " }"
    "}"
    "} ],"
    "yAxes: [ {"
    "display: true,"
    "scaleLabel: {"
    "display: true,"
    "labelString: 'Air Temp F'"
    "}"
    "} ]"
    "}"
    "}"
    "});"
    "</script>"
    //
    "</br>"
    // --- air humidity
    "<div style=\"width: 100%; max-width:600px;padding:10px;background-color:white;\">"
    "<canvas id=\"myChart1\" style=\"width:100%;max-width:600px\"></canvas>"
    "</div>"
    "<script>"
    "new Chart(\"myChart1\", {"
    "  type: \"line\","
    "  data: {"
    "  labels: ["  + time_data_array + "],"
    "datasets: [{ "
    "data: [" + return_float_array_history(air_humidity_5m, index_5m, start_index) + "],"
    "borderColor: \"red\","
    "fill: false"
    "  }]"
    " },"
    "options: {"
    "title: {  display: true, text: 'Air Humidity over Time' },"
    "legend: {display: false},"
    " responsive: true,"
    "scales: {"
    "xAxes: [ {"
    "display: true,"
    "scaleLabel: {"
    "display: true,"
    "labelString: 'Time'"
    "},"
    "ticks: {"
    "major: {"
    "fontStyle: 'bold',"
    "fontColor: '#FF0000'"
    " }"
    "}"
    "} ],"
    "yAxes: [ {"
    "display: true,"
    "scaleLabel: {"
    "display: true,"
    "labelString: 'Air Humidity %'"
    "}"
    "} ]"
    "}"
    "}"
    "});"
    "</script>"
    // ----------------
    "</br>"
    "</div>"
    "</body>"
    "</html>";
}

// run every 5 min
void myHistory_update_history()
{
    air_temp_5m[index_5m] = myTemp_return_air_temp();
    air_humidity_5m[index_5m] = myTemp_return_air_humidity();
    if (return_time_client_status() == true && myWifi_return_connection_status() == 3) // if connected then fill time array
    {
        myTime_return_current_time().toCharArray(time_5m[index_5m], 6);
    }

    if (index_5m++ > 143)
    {
        array_5m_filled = true;
        index_5m = 0;
    }
}

//