// AIConfig.h
#pragma once

struct DifficultySettings {
    float reactionTimeBase;
    float reactionTimeVariance;
    float decisionQuality;
    float executionPrecision;
    float riskTolerance;

    DifficultySettings(float difficulty = 0.8f) {
        // Initialize based on difficulty
        reactionTimeBase = 15.0f - (difficulty * 10.0f);
        reactionTimeVariance = 10.0f - (difficulty * 8.0f);
        decisionQuality = 0.6f + (difficulty * 0.4f);
        executionPrecision = 0.5f + (difficulty * 0.5f);
        riskTolerance = 0.3f + (difficulty * 0.5f);
    }
};

struct BehaviorSettings {
    float centerControlImportance;
    float offenseDefenseBalance;
    float aggressionLevel;
    float adaptabilityLevel;

    BehaviorSettings() :
        centerControlImportance(0.7f),
        offenseDefenseBalance(0.5f),
        aggressionLevel(0.5f),
        adaptabilityLevel(0.5f) {}
};

struct CombatSettings {
    float comboFollowUpThreshold;
    float pummleThreshold;
    float grabAttemptRate;
    float shieldFrequency;

    CombatSettings() :
        comboFollowUpThreshold(0.7f),
        pummleThreshold(0.5f),
        grabAttemptRate(0.3f),
        shieldFrequency(0.4f) {}
};

struct AIConfig {
    DifficultySettings difficulty;
    BehaviorSettings behavior;
    CombatSettings combat;

    AIConfig(float difficultyLevel = 0.8f) :
        difficulty(difficultyLevel) {}

    void SetDifficulty(float difficultyLevel) {
        difficulty = DifficultySettings(difficultyLevel);
    }
};
