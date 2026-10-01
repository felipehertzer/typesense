#include <gtest/gtest.h>
#include "index.h"
#include <vector>
#include <cstring>
#include <limits>
#include <random>
#include <s2/s2loop.h>

/*TEST(IndexTest, PointInPolygon180thMeridian) {
    // somewhere in far eastern russia
    GeoCoord verts[3] = {
        {67.63378886620751, 179.87924212491276},
        {67.6276069384328, -179.8364939577639},
        {67.5749950728145, 179.94421673458666}
    };


    *//*std::vector<S2Point> vertices;
    for(size_t point_index = 0; point_index < 4; point_index++) {
        S2Point vertex = S2LatLng::FromDegrees(verts[point_index].lat, verts[point_index].lon).ToPoint();
        vertices.emplace_back(vertex);
    }

    S2Loop region(vertices);*//*

    Geofence poly1{3, verts};
    double offset = Index::transform_for_180th_meridian(poly1);

    GeoCoord point1 = {67.61896440098865, 179.9998420463554};
    GeoCoord point2 = {67.6332378896519, 179.88828622883355};
    GeoCoord point3 = {67.62717271243574, -179.85954137693625};

    GeoCoord point4 = {67.65842784263879, -179.79268650445243};
    GeoCoord point5 = {67.62016647245217, 179.83764198608083};

    Index::transform_for_180th_meridian(point1, offset);
    Index::transform_for_180th_meridian(point2, offset);
    Index::transform_for_180th_meridian(point3, offset);
    Index::transform_for_180th_meridian(point4, offset);
    Index::transform_for_180th_meridian(point5, offset);

    *//*ASSERT_TRUE(region.Contains(S2LatLng::FromDegrees(point1.lat, point1.lon).ToPoint()));
    ASSERT_TRUE(region.Contains(S2LatLng::FromDegrees(point2.lat, point2.lon).ToPoint()));
    ASSERT_TRUE(region.Contains(S2LatLng::FromDegrees(point3.lat, point3.lon).ToPoint()));
    ASSERT_FALSE(region.Contains(S2LatLng::FromDegrees(point4.lat, point4.lon).ToPoint()));
    ASSERT_FALSE(region.Contains(S2LatLng::FromDegrees(point5.lat, point5.lon).ToPoint()));
*//*
    ASSERT_TRUE(Index::is_point_in_polygon(poly1, point1));
    ASSERT_TRUE(Index::is_point_in_polygon(poly1, point2));
    ASSERT_TRUE(Index::is_point_in_polygon(poly1, point3));

    ASSERT_FALSE(Index::is_point_in_polygon(poly1, point4));
    ASSERT_FALSE(Index::is_point_in_polygon(poly1, point5));
}*/

TEST(IndexTest, GeoPointPackUnpack) {
    std::vector<std::pair<double, double>> latlngs = {
        {43.677223,-79.630556},
        {-0.041935, 65.433296},     // Indian Ocean Equator
        {-66.035056, 173.187202},   // Newzealand
        {-65.015656, -158.336234},  // Southern Ocean
        {84.552144, -159.742483},   // Arctic Ocean
        {84.517046, 171.730040}     // Siberian Sea
    };

    for(auto& latlng: latlngs) {
        int64_t packed_latlng = GeoPoint::pack_lat_lng(latlng.first, latlng.second);
        S2LatLng s2LatLng;
        GeoPoint::unpack_lat_lng(packed_latlng, s2LatLng);
        ASSERT_FLOAT_EQ(latlng.first, s2LatLng.lat().degrees());
        ASSERT_FLOAT_EQ(latlng.second, s2LatLng.lng().degrees());
    }
}

TEST(IndexTest, FloatAsShortDoubleKeepsTheFloatInFewerDigits) {
    size_t short_chars = 0, widened_chars = 0;

    auto check = [&](float value) {
        const double short_value = Index::float_as_short_double(value);
        const float back = static_cast<float>(short_value);
        ASSERT_EQ(0, std::memcmp(&value, &back, sizeof value)) << value;

        const auto stored = nlohmann::json(short_value).dump();
        const auto widened = nlohmann::json(static_cast<double>(value)).dump();
        ASSERT_LE(stored.size(), widened.size()) << stored;
        short_chars += stored.size();
        widened_chars += widened.size();
    };

    for (const float value: {0.0f, -0.0f, 1.0f, -1.0f, 0.1f, 1e-7f, 3.4028235e38f, -3.4028235e38f,
                             std::numeric_limits<float>::min(), std::numeric_limits<float>::denorm_min()}) {
        check(value);
    }

    // Embedding values: most take 17 digits widened, at most 9 here.
    short_chars = widened_chars = 0;
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> embedding_value(-0.5f, 0.5f);
    for (int i = 0; i < 100'000; i++) {
        check(embedding_value(rng));
    }
    ASSERT_LE(short_chars, widened_chars * 65 / 100);

    // Any finite float keeps its exact value.
    std::uniform_int_distribution<uint32_t> any_bits(0, UINT32_MAX);
    for (int i = 0; i < 100'000; i++) {
        uint32_t bits = any_bits(rng);
        float value;
        std::memcpy(&value, &bits, sizeof value);
        if (std::isfinite(value)) {
            check(value);
        }
    }

    // Not finite: returned as is.
    ASSERT_TRUE(std::isinf(Index::float_as_short_double(std::numeric_limits<float>::infinity())));
    ASSERT_TRUE(std::isnan(Index::float_as_short_double(std::numeric_limits<float>::quiet_NaN())));

    ASSERT_EQ("0.10000000149011612", nlohmann::json(static_cast<double>(0.1f)).dump());
    ASSERT_EQ("0.100000001", nlohmann::json(Index::float_as_short_double(0.1f)).dump());
}
