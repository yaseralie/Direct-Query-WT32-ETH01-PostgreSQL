/*
 Navicat Premium Data Transfer

 Source Server         : PostgreSQL_Yaser
 Source Server Type    : PostgreSQL
 Source Server Version : 120004
 Source Host           : localhost:5432
 Source Catalog        : postgres
 Source Schema         : public

 Target Server Type    : PostgreSQL
 Target Server Version : 120004
 File Encoding         : 65001

 Date: 16/04/2024 18:55:56
*/


-- ----------------------------
-- Table structure for dht_data
-- ----------------------------
DROP TABLE IF EXISTS "public"."dht_data";
CREATE TABLE "public"."dht_data" (
  "time" timestamp(6) NOT NULL DEFAULT now(),
  "temp" float4,
  "humidity" float4
)
;
