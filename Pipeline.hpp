#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <vector>
#include <functional>

class Pipeline
{
public:
 /**
     * @brief Adds a step (function) to the pipeline.
     * 
     * The step is added to a list of functions that will be executed in the order they were added.
     * 
     * @param step A callable object (function, lambda, etc.) that represents one stage of the pipeline.
     */
    void addStep(std::function<void()> step);

       /**
     * @brief Executes all steps in the pipeline.
     * 
     * This function goes through each step in the pipeline (in the order they were added) and executes it.
     * 
     * The steps are executed in the order they were added using the `addStep()` method.
     */
    void execute() const;

private:
 /**
     * @brief A list of functions representing the steps in the pipeline.
     * 
     * Each function in this vector corresponds to a step that will be executed when `execute()` is called.
     */
    std::vector<std::function<void()>> steps;
};

#endif // PIPELINE_HPP
