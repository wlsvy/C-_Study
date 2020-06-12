#include <iostream>
#include <vector>
#include <string>
using namespace std;

//new 및 delete 를 언제 바꿔야 좋은 소리를 들을지를 파악해 두자

namespace Item50 {
	/*
		Operator new 와 delete는 상당히 공들여서 만들어진 명령어 인데, 이를 바꾸고 싶다면 반드시 그래야할 이유가 필요하다


		1. 잘못된 힙 사용을 탐지하기 위해
		: 프로그래밍을 하다가 이런 저런 실수를 하다 보면 
		데이터 오버런(overrun, 할당된 메모리 블록의 끝을 넘어 뒤에 기록하는 것) 및 
		언더런(underrun, 할당된 메모리 블록의 시작을 넘어 앞에 기록하는 것) 이 발생할 수도 있습니다. 

		이런 경우에 대비해 사용자 정의 new 를 활용한다면, 
		요구된 크기보다 약간 더 메모리를 할당한 후에 사용자가 실제로 사용할 메모리의 앞과 뒤에 오버런/언더런 탐지용 바이트 패턴을 적어두도록 만들 수 있을 것입니다.
		
		2. 효율을 향상시키기 위해
		: 메모리 할당과 해제가 쓰이는 배경은 너무나도 다양한 나머지, 실제 new/delete 동작은 지극히 보편적이고 온건지향 스타일로 구현되었다. 
		그렇기 때문에 만일 개발자가 자신의 프로그램이 동적 메모리를 어떻게 사용하는지를 제대로 이해하고 있다면 사용자 정의 new/delete를 만들어 쓰는게 더 우수한 성능을 낼 수 있다. 
		'우수한 성능' 이란 실행 속도가 빠르고, 메모리를 적게 차지하는 것.

		3. 동적 할당 메모리의 실제 사용에 관한 통계 정보를 수집하기 위해
		: 응용 프로그램에서 메모리 블록이 어떻게 할당되고 해제되는지 관찰하기 위해서 사용자 정의 new/delete에 로그를 붙이는 방식으로 활용할 수 있다.

		4. 할당 및 해제 속력을 높이기 위해
		: 대표적인 예로 boost의 pool 라이브러리. 클래스 전용(class-specific) 할당자를 사용하는 방법이 있다.
		
		5. 기본 메모리 관리자의 공간 오버헤드를 줄이기 위해
		: 범용 메모리 관리자는 사용자 정의 버전과 비교해서 속력이 느린 경우도 많은데다가 메모리도 많이 잡아먹는 사례가 허다하다.
		
		6. 적당히 타협한 기본 할당자의 바이트 정렬 동작을 보장하기 위해
		: 일부 컴파일러는 new 함수가 double 에 대한 동적할당 시에 8바이트 정렬을 보장하지 않아 속도가 떨어질 수 있다
		사용자 정의 버전을 활용하면 성능을 끌어올릴 수 있다.

		7. 임의의 관계를 맺고 있는 객체들을 한 군데에 나란히 모아 놓기 위해
		: page fault 빈도를 확 줄이기. 공간 지역성 끌어올리기 위해 해당 자료구조를 담을 별도의 힙을 생성

		8. 그때그때 원하는 동작을 수행하도록 하기 위해
		: 응용프로그램의 보안 강화를 위해 해제한 메모리 블록에 0을 덮어 쓰는 사용자 정의 delete 등의 예시가 있다
	*/

	static const int signature = 0xDEADBEEF;
	typedef unsigned char Byte;

	//사용자 정의 operator new 예시. 하지만 문제의 여지가 있는 코드
	void* operator new(std::size_t size) throw(std::bad_alloc) {

		//경계표지 2개를 앞 뒤에 붙일 수 있을 만큼 메모리 크기를 늘린다
		size_t realSize = size + 2 * sizeof(int);

		//malloc을 호출하여 실제 메모리를 얻는다
		void *pMem = malloc(realSize);
		if (!pMem) {
			throw bad_alloc();
		}

		//메모리 표지의 시작과 끝 부분에 경계표지 기록
		*(static_cast<int*>(pMem)) = signature;
		*(reinterpret_cast<int*>(static_cast<Byte*> (pMem) + realSize - sizeof(int))) = signature;

		//앞쪽 경계표지 바로 다음의 메모리를 가리키는 포인터 반환
		return static_cast<Byte*>(pMem) + sizeof(int);
	}

	/*
		여기서 조명되는 문제점이란 바이트 정렬(alignment)
		 : 컴퓨터는 아키텍처(architecture)별로 종류가 다양하며
		 특정 타입의 데이터는 특정 위치의 메모리 지점을 시작 주소로 삼아 저장될 것을 요구사항으로 두고 있다. 

		 이를테면 포인터는 4의 배수에 해당하는 주소에 맞춰서 저장되어야(다시 말해 4바이트 단위로 정렬되어야)하거나 
		 double 값은 8의 배수에 해당되는 주소에 맞추어 저장되어야(즉 8바이트 단위로 정렬되어야) 한다는 것. 
		 
		 모든 new 함수는 어떤 데이터 타입에도 바이트 정렬을 만족하는 포인터를 반환해야 한다는 점에서 바이트 정렬 문제는 중요하다. 
		 즉 위 경우에서는 int 크기만큼만 뒤로 어긋난 주소를 반환하는데 이는 안전하지 않다.

		이식성 문제, 바이트 정렬, 스레드안전성 부분에서 많은 고려를 해봐야 한다.

		사실 new/delete를 직접 구현하는 것은 어려우며, 
		컴파일러에서 제공하는 메모리 관리기능을 사용하는 것이 훨씬 나을 수 있다. 
		
		꽤 많은 플랫폼에서 쓸 수 있는 메모리 관리 함수만을 전문적으로 다루는 상업용 제품들도 출시되어 있다. 
		오픈소스로는 대표적으로 boost 의 메모리 풀.
	*/
}