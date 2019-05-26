#if defined(TEST_LOAD_METHOD_NAME) && defined(TEST_DELAY_METHOD_NAME) && defined(LOG_CONCURRENCY_LEVEL) && defined(LOAD_THRESHOLD) && defined(DELAY_THRESHOLD)
TEST(PerformanceTest, TEST_LOAD_METHOD_NAME) {
    XmlLogger xml;
    xml.load("Log.config");
    constexpr int PERFORMANCE_TEST_NUM_OF_MSG = 40000;
    const char *msg1 = "it is a test";
    const char *msg2 = "and is going to success";
    auto start = std::chrono::high_resolution_clock::now();
    {
        Logger log(xml, 0, LOG_CONCURRENCY_LEVEL);
        for (size_t i = 0; i < PERFORMANCE_TEST_NUM_OF_MSG / 4; i++)
        {
            log.info(msg1);
            log.log(6, msg2);
            log.debug(msg1);
            log.log("info", msg2);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    constexpr double msgPerMilisec = LOAD_THRESHOLD;
    double diff = std::chrono::duration<double, std::milli>(end - start).count();
    std::string error;
    error.append("Log load test.\ntook: ");
    error.append(std::to_string(diff));
    error.append(" milisecond to complete.\nthe freq was: ");
    error.append(std::to_string(PERFORMANCE_TEST_NUM_OF_MSG / diff));
    std::cout << error.c_str() << std::endl;
    ASSERT_TRUE((double) PERFORMANCE_TEST_NUM_OF_MSG / diff > msgPerMilisec);
}
TEST(PerformanceTest, TEST_DELAY_METHOD_NAME) {
    XmlLogger xml;
    xml.load("Log.config");
    Logger log(xml, 0, LOG_CONCURRENCY_LEVEL);
    constexpr int PERFORMANCE_TEST_NUM_OF_MSG = 40000;
    const char *msg1 = "it is a test";
    const char *msg2 = "and is going to success";
    auto sum = std::chrono::high_resolution_clock::duration::zero();
    for (size_t i = 0; i < PERFORMANCE_TEST_NUM_OF_MSG / 4; i++) {
        auto a = std::chrono::high_resolution_clock::now();
        log.info(msg1);
        auto b = std::chrono::high_resolution_clock::now();
        sum += b - a;
        a = std::chrono::high_resolution_clock::now();
        log.log(6, msg2);
        b = std::chrono::high_resolution_clock::now();
        sum += b - a;
        a = std::chrono::high_resolution_clock::now();
        log.debug(msg1);
        b = std::chrono::high_resolution_clock::now();
        sum += b - a;
        a = std::chrono::high_resolution_clock::now();
        log.log("info", msg2);
        b = std::chrono::high_resolution_clock::now();
        sum += b - a;
    }
    constexpr double delayInMili = DELAY_THRESHOLD;
    std::string error;
    error.append("Log delay test.\nthe delay is: ");
    error.append(std::to_string(std::chrono::duration<double, std::milli>(sum).count() / PERFORMANCE_TEST_NUM_OF_MSG));
    std::cout << error.c_str() << std::endl;
    ASSERT_TRUE((std::chrono::duration<double, std::milli>(sum).count() / PERFORMANCE_TEST_NUM_OF_MSG) < delayInMili);
}
#endif
