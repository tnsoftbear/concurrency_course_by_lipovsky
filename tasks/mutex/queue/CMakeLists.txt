begin_task()

set_task_sources(queue_spinlock.hpp)

add_task_test(unit_tests tests/unit.cpp)
add_task_test(stress_tests tests/stress.cpp)

end_task()
