#include <crystal\pworld.h>

using namespace crystal;

ParticleWorld::ParticleWorld(unsigned maxContacts, unsigned iterations)
	:
	resolver(iterations),
	maxContacts(maxContacts)
{
	contacts = new ParticleContact[maxContacts];
	calculateIterations = (iterations == 0);

}

ParticleWorld::~ParticleWorld()
{
	delete[] contacts;
}

void ParticleWorld::startFrame()
{
	for (Particles::iterator p = particles.begin();
		p != particles.end();
		p++)
	{
		// Remove all forces from the accumulator
		(*p)->clearAccumulator();
	}
	
	int usedCount = 0;

	for (ParticleEffect* pe : particleEffects)
	{
		if (!pe->destroyable)
		{
			for (Particle* pep : pe->particles)
			{
				pep->clearAccumulator();
			}
		}
		else
		{
			usedCount++;
		}
	}

	if (usedCount > Particle_Collect_Gap)
	{
		tempList.clear();
		for (int i = 0; i < particleEffects.size(); i++)
		{
			if (particleEffects[i]->destroyable)
			{
				delete particleEffects[i];
			}
			else
			{
				tempList.push_back(particleEffects[i]);
			}
		}
		particleEffects = tempList;
	}
}

unsigned ParticleWorld::generateContacts()
{
	unsigned limit = maxContacts;
	ParticleContact *nextContact = contacts;

	for (ContactGenerators::iterator g = contactGenerators.begin();
		g != contactGenerators.end();
		g++)
	{
		unsigned used = (*g)->addContact(nextContact, limit);
		limit -= used;
		nextContact += used;

		// We've run out of contacts to fill. This means we're missing
		// contacts.
		if (limit <= 0) break;
	}

	// Return the number of contacts used.
	return maxContacts - limit;
}

void ParticleWorld::integrate(real duration)
{
	for (Particles::iterator p = particles.begin();
		p != particles.end();
		p++)
	{
		// Remove all forces from the accumulator
		(*p)->integrate(duration);
	}
	for (ParticleEffect* pe : particleEffects)
	{
		if (!pe->destroyable) 
		{
			for (Particle* pep : pe->particles)
			{
				pep->integrate(duration);
			}
		}	
	}
}

void ParticleWorld::runPhysics(real duration)
{
	// First apply the force generators
	registry.updateForces(duration);

	// Then integrate the objects
	integrate(duration);

	// Generate contacts
	unsigned usedContacts = generateContacts();

	// And process them
	if (usedContacts)
	{
		if (calculateIterations) resolver.setIterations(usedContacts * 2);
		resolver.resolveContacts(contacts, usedContacts, duration);
	}
}

ParticleWorld::Particles& ParticleWorld::getParticles()
{
	return particles;
}

ParticleWorld::ContactGenerators& ParticleWorld::getContactGenerators()
{
	return contactGenerators;
}

ParticleForceRegistry& ParticleWorld::getForceRegistry()
{
	return registry;
}

void GroundContacts::init(crystal::ParticleWorld::Particles *particles)
{
	GroundContacts::particles = particles;
}

unsigned GroundContacts::addContact(crystal::ParticleContact *contact,
	unsigned limit) const
{
	unsigned count = 0;
	for (crystal::ParticleWorld::Particles::iterator p = particles->begin();
		p != particles->end();
		p++)
	{
		crystal::real y = (*p)->getPosition().y;
		if (y < 0.0f)
		{
			contact->contactNormal = crystal::Vector3::UP;
			contact->particle[0] = *p;
			contact->particle[1] = NULL;
			contact->penetration = -y;
			contact->restitution = 0.2f;
			contact++;
			count++;
		}

		if (count >= limit) return count;
	}
	return count;
}

void ParticleWorld::addParticleEffect(ParticleEffect* p)
{
	particleEffects.push_back(p);
}