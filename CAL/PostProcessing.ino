void enterPostProcessing()
{
    playModeMusic(6);

    tft.fillScreen(TFT_WHITE);

    tft.setTextColor(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    tft.drawString("POST PROCESSING", 240, 30, 4);
}

void postProcessingLoop()
{
    static bool firstRun = true;

    if(firstRun)
    {
        firstRun = false;
        enterPostProcessing();
    }

    // code xử lý sau này
}